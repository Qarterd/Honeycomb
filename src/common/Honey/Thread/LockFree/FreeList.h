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
    
    FreeList(szt size = 0)                          : _pool({make_tuple(sizeof(T), size)}) {}
    
    template<class... Args>
    T* construct(Args&&... args)                    { return new (_pool.alloc(sizeof(T))) T(forward<Args>(args)...); }
    
    void destruct(T* ptr)                           { assert(ptr); ptr->~T(); _pool.free(ptr); }
    
    Handle handle(T* ptr) const
    {
        if (!ptr) return nullptr;
        MemPool::Bucket::BlockHeader* header = MemPool::Bucket::blockHeader(ptr);
        header->validate(MemPool::Bucket::BlockHeader::Debug::sigUsed);
        return header->handle;
    }
    
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
    FreeListAllocator(const FreeListAllocator<U>&)  {}

    MemPool& pool()                                 { return _freeList._pool; }
    
    FreeList<T> _freeList;
};

} }
