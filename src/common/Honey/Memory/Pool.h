// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/MtMap.h"
#include "Honey/Thread/Lock/Spin.h"

namespace honey
{

/// \addtogroup Memory
/// @{

/// Memory pool
/**
  * A pool instance is constructed via MemPool::Factory.
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
public:
    /// Pool creator
    class Factory
    {
        friend class MemPool;
    public:
        Factory()                                           : param(mtmap(align() = alignof(double))) {}

        /// Create pool using factory config
        MemPool& create()                                   { return *new MemPool(*this); }

        /// Set alignment byte boundary for all blocks. Alignment must be a power of two.
        void setBlockAlign(szt bytes)                       { param[align()] = bytes; }

        /// Add a bucket of blocks available to the pool for allocation
        /**
          * \param blockSize    size in bytes of each block.
          * \param blockCount   initial number of blocks (bucket automatically expands).
          */
        void addBucket(szt blockSize, szt blockCount)       { bucketList.push_back(mtmap(Factory::blockSize() = blockSize, Factory::blockCount() = blockCount)); }

    private:
        mtkey(align);
        MtMap<szt, align> param;

        mtkey(blockSize);
        mtkey(blockCount);
        vector<MtMap<szt, blockSize, szt, blockCount>> bucketList;
    };
    friend class Factory;

    ~MemPool()
    {
        deleteRange(_bucketList);
        delete_(_bucketChunk);
        delete_(_heap);
    }

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

    void setId(const Id& id)                                { _id = id; }
    const Id& getId() const                                 { return _id; }

private:
    /// Bucket that holds a number of blocks of fixed size
    class Bucket
    {
    public:
        /// Blocks are linked via indices rather than pointers so that they can include a tag while still maintaining a swappable size
        struct Handle
        {
            //divide max swappable into 2 parts
            typedef mt::uintBySize<sizeof(atomic::SwapMaxType)/2>::type Int;
            
            Handle()                                        : index(-1) {}
            Handle(nullptr_t)                               : Handle() {}
            Handle(uint8 chunk, Int block)                  : index((block << 8) | chunk) {}
            
            bool operator==(Handle handle) const            { return index == handle.index; }
            bool operator!=(Handle handle) const            { return !operator==(handle); }
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
            
            TaggedHandle& operator=(Handle handle)          { Handle::operator=(handle); return *this; }
            
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
            _chunkSizeTotal(0),
            _freeHead(TaggedHandle()),
            _freeCount(0),
            _usedCount(0),
            _usedSize(0)
        {}

        ~Bucket()
        {
            //The first memory chunk is the initial pool allocation, we don't own it
            _chunkList.erase(_chunkList.begin());
            //Delete all expansion chunks
            for (auto& e : _chunkList) delete_(e.data());
        }

        /// Initialize blocks in memory chunk
        void initChunk(uint8* chunk, szt chunkSize, szt blockCount);
        /// Alloc a block with alignment byte boundary `align`
        void* alloc(szt size, uint8 align, const char* srcFile, int srcLine);
        /// Increase number of blocks in bucket by allocating a new chunk
        void expand();
        /// Free a block
        void free(BlockHeader* header);

        szt blockOffsetMax() const                          { return _pool._blockAlign-1; }
        szt blockStride() const                             { return (szt)(intptr_t)alignCeil((void*)(_blockSize + sizeof(BlockHeader)), _pool._blockAlign); }
        
        /// Get block header from handle
        BlockHeader* deref(Handle handle) const
        {
            assert(handle && handle.chunk() < _chunkList.size());
            auto& chunk = _chunkList[handle.chunk()];
            assert(blockStride()*handle.block() < chunk.size());
            uint8* blockData = alignCeil(chunk.data() + sizeof(BlockHeader), _pool._blockAlign);
            return blockHeader(blockData + blockStride()*handle.block());
        }
        
        MemPool&                _pool;
        uint8                   _bucketIndex;
        const szt               _blockSize;         ///< Data size of each block
        const szt               _blockCountInit;    ///< Initial number of blocks
        vector<Buffer<uint8>>   _chunkList;         ///< System heap chunks
        Atomic<szt>             _chunkSizeTotal;    ///< Total number of bytes allocated from system heap
        Atomic<TaggedHandle>    _freeHead;          ///< Head of free blocks list
        Atomic<szt>             _freeCount;         ///< Number of free blocks
        TaggedHandle            _usedHead;          ///< Head of used blocks list
        Atomic<szt>             _usedCount;         ///< Number of used blocks
        szt                     _usedSize;          ///< Total number of bytes allocated in used blocks
        SpinLock                _lock;
    };
    typedef std::map<szt, Bucket*> BucketMap;

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
            _chunkSizeTotal(0),
            _usedHead(nullptr),
            _usedCount(0)
        {}

        /// Alloc an `size` bytes block with alignment byte boundary `align`
        void* alloc(szt size, uint8 align, const char* srcFile, int srcLine);
        /// Free a block
        void free(BlockHeader* header);

        MemPool&            _pool;
        Atomic<szt>         _chunkSizeTotal;    ///< Total number of bytes allocated from system heap
        BlockHeader*        _usedHead;          ///< Head of used blocks list
        Atomic<szt>         _usedCount;         ///< Number of used blocks
        SpinLock            _lock;
    };

    MemPool(const Factory& factory);

    #ifdef DEBUG
        void lock() const;
        void unlock() const;
    #endif
    
    Id                      _id;
    const szt               _blockAlign;        ///< alignment of all blocks
    szt                     _blockSizeMax;      ///< Maximum block size
    vector<Bucket*>         _bucketList;
    BucketMap               _bucketMap;         ///< Buckets ordered by size
    uint8*                  _bucketChunk;       ///< Initial contiguous chunk of memory for all buckets, allocated from system heap
    Heap*                   _heap;              ///< Heap allocator
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
