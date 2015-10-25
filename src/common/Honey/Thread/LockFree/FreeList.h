// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Memory/Pool.h"

namespace honey { namespace lockfree
{

/// Lock-free free list, allocates objects and provides memory reclamation and automatic storage expansion. Based on MemPool.
template<class T>
class FreeList : mt::NoCopy
{
    template<class> friend class FreeListAllocator;
public:
    typedef T value_type;
    typedef MemPool::Bucket::Handle Handle;
    typedef MemPool::Bucket::TaggedHandle TaggedHandle;
    
    FreeList(szt capacity = 0)                      : _pool({make_tuple(sizeof(T), capacity)}) {}
    
    /// Ensure that enough storage is allocated for a number of objects
    void reserve(szt capacity)                      { _pool._buckets[0]->reserve(capacity); }
    
    szt capacity() const                            { return _pool._buckets[0]->_blockCount; }
    
    /// Construct object and remove from free list
    template<class... Args>
    T* construct(Args&&... args)                    { return new (_pool.alloc(sizeof(T))) T(forward<Args>(args)...); }
    
    /// Destroy object and add to free list
    void destroy(T* ptr)                            { assert(ptr); ptr->~T(); _pool.free(ptr); }
    
    /// Get lock-free handle for object
    Handle handle(T* ptr) const
    {
        if (!ptr) return nullptr;
        MemPool::Bucket::BlockHeader* header = MemPool::Bucket::blockHeader(ptr);
        header->validate(MemPool::Bucket::BlockHeader::Debug::sigUsed);
        return header->handle;
    }
    
    /// Get object from lock-free handle
    T* deref(Handle handle) const                   { return reinterpret_cast<T*>(MemPool::Bucket::blockData(_pool._buckets[0]->deref(handle))); }
    
private:
    MemPool _pool;
};

template<class T>
class FreeListAllocator : public MemPoolAllocator<FreeListAllocator, T>
{
public:
    FreeListAllocator() = default;
    template<class U>
    FreeListAllocator(const FreeListAllocator<U>&) {}

    MemPool& pool()                                 { return _freeList._pool; }
    
private:
    FreeList<T> _freeList;
};

} }
