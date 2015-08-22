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
  * A pool instance is constructed by using the Factory class. \n
  * A pool instance can handle allocations of any size and alignment. \n
  * The pool primarily allocates from buckets of fixed size blocks, if there's no block big enough to hold the allocation then the pool falls back on the system heap allocator. \n
  * The pool is thread-safe, each bucket can concurrently run Alloc and Free (i.e. Alloc does not block Free).
  *
  * The pool will initially allocate the memory for all its buckets/blocks in one contiguous memory chunk. \n
  * Each bucket has a fixed block size and an initial block count. \n
  * The buckets automatically expand like vectors, but the memory chunks are not contiguous across expansions.
  *
  * Alloc complexity is O(log B) amortized, where B is the number of buckets, expansions may occur. \n
  * Free complexity is O(1). \n
  * Destruction complexity is O(B + N) where B is the number of buckets, N is the number of blocks allocated by the heap allocator.
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

        /// Set alignment byte boundary for all blocks.  alignment must be a power of two.
        void setBlockAlign(szt bytes)                       { param[align()] = bytes; }

        /// Add a bucket of blocks available to the pool for allocation
        /**
          * \param blockSize_   size in bytes of each block.
          * \param blockCount_  initial number of blocks (bucket automatically expands).
          */
        void addBucket(szt blockSize_, szt blockCount_)     { bucketList.push_back(mtmap(blockSize() = blockSize_, blockCount() = blockCount_)); }

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
    void* alloc(szt size, szt align = 1, const char* srcFile = nullptr, int srcLine = 0);

    /// Free a memory block allocated from the pool
    void free(void* ptr_);

    /// Free entire pool
    void free()
    {
        for (auto& e : _bucketList) { e->free(); }
        _heap->free();
    }

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
    class Bucket;

    /// Bucket block header, 8 bytes.  Block header types are determined by examining the 4 bytes prior to the block data.
    struct BlockHeader
    {
        /// Info only available in debug
        struct DebugInfo
        {
            szt                 size;
            int                 srcLine;
            const char*         srcFile;
            BlockHeader*        prev;
            int                 _pad;
            int                 sig;                ///< Signature sentinel to verify block state
        };

        static const int sigFree = 0xB10CF8EE;      ///< Block Free
        static const int sigUsed = 0x05EDB10C;      ///< Used Block

        #ifdef DEBUG
            /// Assert that block signature is valid and matches expected value `sig`
            void validate(int sig)
            {
                assert(debug.sig == sigFree || debug.sig == sigUsed, "Error: invalid block signature. Block header overwritten.");
                assert(debug.sig == sig, "Error: invalid block signature. Block not in expected state (eg. block freed twice).");
            }
        #else
            void validate(int) {}
        #endif

        int16           bucket;
        uint16          offset;    ///< Offset from original block position due to alignment (can change each allocation)
        #ifdef DEBUG
            DebugInfo   debug;
        #endif
        BlockHeader*    next;
    };
    

    static BlockHeader* blockHeader(uint8* data)            { return reinterpret_cast<BlockHeader*>(data - sizeof(BlockHeader)); }
    static uint8* blockData(BlockHeader* header)            { return reinterpret_cast<uint8*>(header) + sizeof(BlockHeader); }

    /// Bucket that holds a number of blocks of fixed size
    class Bucket
    {
    public:
        Bucket(MemPool& pool, szt blockSize, szt blockCount) :
            _pool(pool),
            _bucketIndex(-1),
            _blockSize(blockSize),
            _blockCountInit(blockCount),
            _chunkSizeTotal(0),
            _freeHead(nullptr),
            _freeTail(nullptr),
            _freeCount(0),
            _usedHead(nullptr),
            _usedCount(0),
            _usedSize(0)
        {}

        ~Bucket()
        {
            //The first memory chunk is the initial pool allocation, we don't own it
            _chunkList.erase(_chunkList.begin());
            //Delete all expansion chunks
            deleteRange(_chunkList);
        }

        /// Initialize blocks in memory chunk
        void initChunk(uint8* chunk, szt chunkSize, szt blockCount);

        /// Alloc a block with alignment byte boundary `align`
        void* alloc(szt size, szt align, const char* srcFile, int srcLine);

        /// Increase number of blocks in bucket by allocating a new chunk
        void expand();

        /// Free a block
        void free(BlockHeader* header);

        /// Free entire bucket
        void free()                                         { for (auto header = _usedHead; header; header = _usedHead) free(header); }

        szt blockStride() const                             { return (szt)(intptr_t)alignCeil((void*)(_blockSize + sizeof(BlockHeader)), _pool._blockAlign); }
        szt blockOffsetMax() const                          { return _pool._blockAlign-1; }

        MemPool&            _pool;
        szt                 _bucketIndex;
        const szt           _blockSize;         ///< Data size of each block
        const szt           _blockCountInit;    ///< Initial number of blocks
        vector<uint8*>      _chunkList;         ///< System heap chunks
        atomic::Var<szt>    _chunkSizeTotal;    ///< Total number of bytes allocated from system heap
        BlockHeader*        _freeHead;          ///< Head of free blocks list
        BlockHeader*        _freeTail;          ///< Tail of free blocks list
        atomic::Var<szt>    _freeCount;         ///< Number of free blocks
        BlockHeader*        _usedHead;          ///< Head of used blocks list
        atomic::Var<szt>    _usedCount;         ///< Number of used blocks
        szt                 _usedSize;          ///< Total number of bytes allocated in used blocks
        SpinLock            _lock;
        SpinLock            _tailLock;
    };
    typedef std::map<szt, Bucket*> BucketMap;

    /// Allocator that wraps blocks allocated from the system heap
    class Heap
    {
    public:
        /// Heap block header, 8 bytes + base
        struct BlockHeader : MemPool::BlockHeader
        {
            typedef MemPool::BlockHeader Super;

            BlockHeader* next_()                            { return static_cast<BlockHeader*>(Super::next); }

            szt     size;
            Heap*   heap;
        };

        static BlockHeader* blockHeader(uint8* data)        { return reinterpret_cast<BlockHeader*>(data - sizeof(BlockHeader)); }
        static uint8* blockData(BlockHeader* header)        { return reinterpret_cast<uint8*>(header) + sizeof(BlockHeader); }

        Heap(MemPool& pool) :
            _pool(pool),
            _chunkSizeTotal(0),
            _usedHead(nullptr),
            _usedCount(0)
        {}

        ~Heap()                                             { free(); }

        /// Alloc an `size` bytes block with alignment byte boundary `align`
        void* alloc(szt size, szt align, const char* srcFile, int srcLine);

        /// Free a block
        void free(BlockHeader* header);

        /// Free entire bucket
        void free()                                         { for (auto header = _usedHead; header; header = _usedHead) free(header); }

        MemPool&            _pool;
        atomic::Var<szt>    _chunkSizeTotal;    ///< Total number of bytes allocated from system heap
        BlockHeader*        _usedHead;          ///< Head of used blocks list
        atomic::Var<szt>    _usedCount;         ///< Number of used blocks
        SpinLock            _lock;
    };

    MemPool(const Factory& factory);

    void lock() const
    {
        for (auto& e : _bucketList) { e->_lock.lock(); e->_tailLock.lock(); }
        _heap->_lock.lock();
    }

    void unlock() const
    {
        for (auto& e : _bucketList) { e->_lock.unlock(); e->_tailLock.unlock(); }
        _heap->_lock.unlock();
    }
    
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
    
    pointer allocate(size_type n, const void* = 0)                  { return static_cast<pointer>(this->subc().pool().alloc(sizeof(T)*n)); }
    pointer allocate(size_type n, const char* srcFile, int srcLine, const void* = 0)
                                                                    { return static_cast<pointer>(this->subc().pool().alloc(sizeof(T)*n, 1, srcFile, srcLine)); }
    void deallocate(pointer p, size_type)                           { this->subc().pool().free(p); }
};

/// @}

}
