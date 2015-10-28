// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/MtMap.h"
#include "Honey/Thread/Lock/Spin.h"

namespace honey
{

namespace lockfree { template<class> class FreeList; }

/// \addtogroup Memory
/// @{

/// Memory pool
/**
  * The pool primarily allocates from buckets of fixed size blocks, if there's no block big enough to hold
  * the allocation then the pool falls back on the system heap allocator.
  *
  * The pool will initially allocate memory for all its buckets in one contiguous chunk.
  * Buckets automatically expand but their chunks are not contiguous across expansions.
  *
  * The pool is thread-safe and its buckets are lock-free, although locks may be encountered
  * during allocation when bucket expansion is required.
  *
  * On platforms with a 64-bit atomic swap the pool supports 2^24 blocks per bucket.
  * Alloc complexity is O(log B) where B is the number of buckets. Free complexity is O(1).
  */
class MemPool : mt::NoCopy
{
    template<class> friend class lockfree::FreeList;
public:
    /**
      * \param buckets  A vector of tuples (blockSize, blockCount).
      * \param id       display id for debugging
      *                 Buckets of blocks available to the pool for allocation.
      * \param align    Alignment byte boundary for all blocks. Alignment must be a power of two.
      */
    MemPool(const vector<tuple<szt,szt>>& buckets, const Id& id = idnull, szt align = alignof(double));
    
    /// Allocate a `size` bytes block of memory at byte boundary `align`.  alignment must be a power of two.
    void* alloc(szt size, uint8 align = 1, const char* srcFile = nullptr, int srcLine = 0);
    /// Free a memory block allocated from the pool
    void free(void* ptr_);

    /// Calc total bytes allocated by pool
    szt allocBytes() const;
    /// Calc total bytes used in pool
    szt usedBytes() const;
    /// Calc total bytes free in pool
    szt freeBytes() const                                   { return allocBytes() - usedBytes(); }

    #ifdef DEBUG
        /// Ensure that all used/free blocks are valid (check signatures)
        void validate() const;
        /// Print statistics about pool
        String printStats() const;
        /// Print all used blocks
        String printUsed() const;
    #else
        void validate() const                               {}
        String printStats() const                           { return ""; }
        String printUsed() const                            { return ""; }
    #endif

    const Id& id() const                                    { return _id; }

private:
    /// Bucket that holds a number of blocks of fixed size
    class Bucket
    {
    public:
        /// Enables blocks to be referenced via index rather than pointer so that they can include a tag while still maintaining a swappable size
        /** Each chunk holds an exponential expansion of a bucket, so a 1-byte chunk index is sufficient. */
        struct Handle
        {
            //divide max swappable into 2 parts, index and tag
            typedef mt::uintBySize<sizeof(atomic::SwapMaxType)/2>::type Int;
            
            Handle()                                        : index(-1) {}
            Handle(nullptr_t)                               : Handle() {}
            Handle(uint8 chunk, Int block)                  : index((block << 8) | chunk) {}
            
            bool operator==(Handle rhs) const               { return index == rhs.index; }
            bool operator!=(Handle rhs) const               { return !operator==(rhs); }
            explicit operator bool() const                  { return index != Int(-1); }
            
            uint8 chunk() const                             { return index & 0xFF; }
            Int block() const                               { return index >> 8; }
            
            Int index;
        };
        
        /// Holds block handle and tag to prevent lock-free ABA issues
        struct TaggedHandle : Handle
        {
            TaggedHandle()                                  : tag(0) {}
            TaggedHandle(Handle handle, Int tag)            : Handle(handle), tag(tag) {}
            
            TaggedHandle& operator=(Handle rhs)             { Handle::operator=(rhs); return *this; }
            
            bool operator==(TaggedHandle rhs) const         { return Handle::operator==(rhs) && tag == rhs.tag; }
            bool operator!=(TaggedHandle rhs) const         { return !operator==(rhs); }
            
            const Handle& handle() const                    { return *this; }
            Handle& handle()                                { return *this; }
            Int nextTag() const                             { return tag+1; }
            
            Int tag;
        };
        
        /// Bucket block header
        struct BlockHeader
        {
            struct Debug
            {
                static const int sigFree = 0xB10CF8EE;  ///< Block Free
                static const int sigUsed = 0x05EDB10C;  ///< Used Block
            
                szt             size;
                int             srcLine;
                const char*     srcFile;
                Handle          prev;
                int             sig;    ///< Signature sentinel to verify block state
            };

            /// Get bucket from reserved area
            uint8& bucket()                                 { return *(reinterpret_cast<uint8*>(this) + sizeof(BlockHeader) - 1); }
            
            #ifdef DEBUG
                /// Assert that block signature is valid and matches expected value `sig`
                void validate(int sig)                      { assert(debug.sig == sig, "Error: invalid block signature. Block overwritten by overflow or in unexpected state (eg. freed twice)."); }
            #else
                void validate(int) {}
            #endif

            #ifdef DEBUG
                Debug       debug;
            #endif
            Handle          handle;
            Handle          next;
            uint8           offset;     ///< Offset from original block position due to alignment (can change each allocation)
            uint8           reserved;   ///< Last byte is reserved to differentiate block header types
        };
        
        static BlockHeader* blockHeader(uint8* data)        { return reinterpret_cast<BlockHeader*>(data - sizeof(BlockHeader)); }
        static uint8* blockData(BlockHeader* header)        { return reinterpret_cast<uint8*>(header) + sizeof(BlockHeader); }
        
        Bucket(MemPool& pool, szt blockSize, szt blockCount) :
            _pool(pool),
            _bucketIndex(-1),
            _blockSize(blockSize),
            _blockCountInit(blockCount),
            _blockCount(0),
            _chunkCount(0),
            _chunkSizeTotal(0),
            _freeHead(TaggedHandle()),
            _freeCount(0),
            _usedCount(0),
            _usedSize(0)
        {}

        ~Bucket()
        {
            //Delete all expansion chunks. The first chunk is the initial pool allocation, we don't own it.
            for (auto i : range(1, _chunkCount.load())) delete_(_chunks[i].data());
        }

        /// Initialize blocks in memory chunk
        void initChunk(uint8* chunk, szt chunkSize, szt blockCount);
        /// Alloc a block with alignment byte boundary `align`
        void* alloc(szt size, uint8 align, const char* srcFile, int srcLine);
        /// Ensure that there are a number of blocks available
        void reserve(szt capacity);
        /// Exponentially increase number of blocks in bucket
        void expand();
        /// Free a block
        void free(BlockHeader* header);

        szt blockOffsetMax() const                          { return _pool._blockAlign-1; }
        szt blockStride() const                             { return (szt)(intptr_t)alignCeil((void*)(_blockSize + sizeof(BlockHeader)), _pool._blockAlign); }
        
        /// Get block header from handle
        BlockHeader* deref(Handle handle) const
        {
            assert(handle && handle.chunk() < _chunkCount);
            auto& chunk = _chunks[handle.chunk()];
            assert(blockStride()*handle.block() < chunk.size());
            uint8* blockData = alignCeil(chunk.data() + sizeof(BlockHeader), _pool._blockAlign);
            return blockHeader(blockData + blockStride()*handle.block());
        }
        
        MemPool&                _pool;
        uint8                   _bucketIndex;
        const szt               _blockSize;
        const szt               _blockCountInit;
        szt                     _blockCount;
        array<Buffer<uint8>, numeral<uint64>().sizeBits> _chunks;    ///< System heap chunks, this array is small as chunks grow exponentially
        Atomic<uint8>           _chunkCount;
        Atomic<szt>             _chunkSizeTotal;    ///< Total number of bytes allocated from system heap
        Atomic<TaggedHandle>    _freeHead;          ///< Head of free blocks list
        Atomic<szt>             _freeCount;
        TaggedHandle            _usedHead;          ///< Head of used blocks list
        Atomic<szt>             _usedCount;
        szt                     _usedSize;          ///< Total number of bytes allocated in used blocks
        SpinLock                _lock;
    };

    /// Allocator that wraps blocks allocated from the system heap
    class Heap
    {
    public:
        /// Heap block header
        struct BlockHeader
        {
            struct Debug
            {
                int             srcLine;
                const char*     srcFile;
                BlockHeader*    prev;
                int             sig;        ///< Signature sentinel to verify block state
            };

            static const uint8 heapTag = -1;
            
            /// Get tag from reserved area
            uint8& tag()                                    { return *(reinterpret_cast<uint8*>(this) + sizeof(BlockHeader) - 1); }
            
            #ifdef DEBUG
                /// Assert that block signature is valid and matches expected value `sig`
                void validate(int sig)                      { assert(debug.sig == sig, "Error: invalid block signature. Block overwritten by overflow or in unexpected state (eg. freed twice)."); }
            #else
                void validate(int) {}
            #endif
            
            #ifdef DEBUG
                Debug           debug;
            #endif
            BlockHeader*        next;
            szt                 size;
            uint8               offset;     ///< Offset from original block position due to alignment (can change each allocation)
            uint8               reserved;   ///< Last byte is reserved to differentiate block header types
        };

        static BlockHeader* blockHeader(uint8* data)        { return reinterpret_cast<BlockHeader*>(data - sizeof(BlockHeader)); }
        static uint8* blockData(BlockHeader* header)        { return reinterpret_cast<uint8*>(header) + sizeof(BlockHeader); }

        Heap(MemPool& pool) :
            _pool(pool),
            _allocTotal(0),
            _usedHead(nullptr),
            _usedCount(0)
        {}

        /// Alloc an `size` bytes block with alignment byte boundary `align`
        void* alloc(szt size, uint8 align, const char* srcFile, int srcLine);
        /// Free a block
        void free(BlockHeader* header);

        MemPool&            _pool;
        Atomic<szt>         _allocTotal;        ///< Total number of bytes allocated from system heap
        BlockHeader*        _usedHead;          ///< Head of used blocks list
        Atomic<szt>         _usedCount;
        SpinLock            _lock;
    };

    #ifdef DEBUG
        void lock() const;
        void unlock() const;
    #endif
    
    Id                          _id;
    const szt                   _blockAlign;        ///< alignment of all blocks
    szt                         _blockSizeMax;
    vector<UniquePtr<Bucket>>   _buckets;
    std::map<szt, Bucket*>      _bucketMap;         ///< Buckets ordered by size
    UniquePtr<uint8>            _bucketChunk;       ///< Initial contiguous chunk of memory for all buckets, allocated from system heap
    UniquePtr<Heap>             _heap;
};

/// MemPool allocator
/**
  * Subclass must define:
  * - default/copy/copy-other ctors
  * - MemPool& pool()
  */
template<template<class> class Subclass, class T>
class MemPoolAllocator : public Allocator<Subclass, T>
{
    typedef Allocator<Subclass, T> Super;
public:
    using typename Super::pointer;
    using typename Super::size_type;
    
    pointer allocate(size_type n, const void* = 0)          { return static_cast<pointer>(this->subc().pool().alloc(sizeof(T)*n)); }
    pointer allocate(size_type n, const char* srcFile, int srcLine, const void* = 0)
                                                            { return static_cast<pointer>(this->subc().pool().alloc(sizeof(T)*n, 1, srcFile, srcLine)); }
    void deallocate(pointer p, size_type)                   { this->subc().pool().free(p); }
};

/// @}

}
