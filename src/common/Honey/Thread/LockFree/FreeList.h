// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Memory/Pool.h"

namespace honey { namespace lockfree
{

/// Lock-free freelist, allocates re-usable objects and provides automatic storage expansion for concurrent algorithms
/**
  * Memory is only reclaimed upon destruction.
  *
  * \see HazardMem for lock-free memory reclamation.
  */
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
    /// The number of objects for which storage is allocated
    szt capacity() const                            { return _pool._buckets[0]->_blockCount; }
    
    /// Remove object from free list without constructing it
    T* alloc()                                      { return static_cast<T*>(_pool.alloc(sizeof(T))); }
    /// Construct object and remove from free list
    template<class... Args>
    T* construct(Args&&... args)                    { return new (alloc()) T{forward<Args>(args)...}; }
    
    /// Add object to free list without destroying it
    void free(T* ptr)                               { assert(ptr); _pool.free(ptr); }
    /// Destroy object and add to free list
    void destroy(T* ptr)                            { assert(ptr); ptr->~T(); free(ptr); }
    
    /// Get compressed handle for object
    Handle handle(T* ptr) const                     { return ptr ? MemPool::Bucket::blockHeader(reinterpret_cast<uint8*>(ptr))->handle : nullptr; }
    /// Get object from compressed handle
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
