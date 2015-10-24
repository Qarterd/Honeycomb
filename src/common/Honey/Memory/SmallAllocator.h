// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Memory/Pool.h"

namespace honey
{

/// \addtogroup Memory
/// @{

MemPool* SmallAllocator_createSingleton();
/** \cond */
namespace priv
{
    //hold the pool as a reference so that it's never destroyed, as other static objects depend on it
    inline MemPool& SmallAllocator_pool()               { static MemPool& inst = *SmallAllocator_createSingleton(); return inst; }
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
    inline MemPool* SmallAllocator_createSingleton()
    {
        return new MemPool(
            {
                make_tuple(8, 2000),
                make_tuple(16, 2000),
                make_tuple(32, 1000),
                make_tuple(64, 500),
                make_tuple(128, 200),
                make_tuple(256, 100),
                make_tuple(512, 50)
            }, "Small"_id);
    }
#endif

/// @}

}
