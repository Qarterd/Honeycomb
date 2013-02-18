// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Memory/Pool.h"

namespace honey
{

/// \addtogroup Memory
/// @{

MemPool& SmallAllocator_createSingleton();
/** \cond */
namespace priv
{
    inline MemPool& SmallAllocator_pool()               { static MemPool& inst = SmallAllocator_createSingleton(); return inst; }
}
/** \endcond */

/// Global allocator for small memory blocks.  To provide a custom pool define `SmallAllocator_createSingleton_` and implement SmallAllocator_createSingleton().
template<class T>
class SmallAllocator : public MemPoolAllocator<SmallAllocator, T>
{
public:
    SmallAllocator() = default;
    template<class U>
    SmallAllocator(const SmallAllocator<U>&) {}

    static MemPool& pool()                              { return priv::SmallAllocator_pool(); }
};

/// Inherit from this class to use the small block allocator
typedef AllocatorObject<SmallAllocator> SmallAllocatorObject;

#ifndef SmallAllocator_createSingleton_
    /// Default implementation
    inline MemPool& SmallAllocator_createSingleton()
    {
        MemPool::Factory factory;
        factory.addBucket(8, 5000);
        factory.addBucket(16, 2000);
        factory.addBucket(32, 2000);
        factory.addBucket(64, 2000);
        factory.addBucket(128, 500);
        factory.addBucket(256, 100);
        factory.addBucket(512, 50);
        auto& pool = factory.create();
        pool.setId("Small"_id);
        return pool;
    }
#endif

/// @}

}
