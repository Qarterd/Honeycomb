// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include <intrin.h>

/** \cond */
namespace honey { namespace atomic { namespace platform
{

/// x86 atomic ops
/**
  * x86 uses a strong memory model:
  * - Loads are not reordered with other loads. 
  * - Stores are not reordered with other stores. 
  * - Stores are not reordered with older loads.
  * - Loads may be reordered with older stores to different locations but not with older stores to the same location.  
  * - In a multiprocessor system, memory ordering obeys causality (memory ordering respects transitive visibility). 
  * - In a multiprocessor system, stores to the same location have a total order.  
  * - In a multiprocessor system, locked instructions have a total order. 
  * - Loads and stores are not reordered with locked instructions.
  */
class Op
{
public:
    static int32 load(volatile const int32& val, Order::t o)
    {
        switch(o)
        {
        case Order::relaxed:
            return val;                //Read is atomic if val is 32-bit aligned
        case Order::consume:
        case Order::acquire:
            {
                const int32 _val = val;
                mt_unused(_val);
                _ReadWriteBarrier();    //Prevent compiler moving operations from after acquire to before
                return val;
            }
        case Order::seqCst:
        default:
            MemoryBarrier();            //Prevent compiler/hardware re-ordering
            return val;
        }
    }
    
    static void store(volatile int32& dst, int32 newVal, Order::t o)
    {
        switch (o)
        {
        case Order::relaxed:
            dst = newVal;              //Write is atomic if val is 32-bit aligned
            break;
        case Order::release:
            _ReadWriteBarrier();        //Prevent compiler moving operations from before release to after
            dst = newVal;
            break;
        case Order::seqCst:
        default:
            swap(dst, newVal);         //Swap provides memory barrier
            break;
        }
    }

    static bool cas(volatile int32& dst, int32 newVal, int32 cmp, Order::t)         { return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&dst), newVal, cmp) == cmp; }
    /// Optimized methods
    static int32 swap(volatile int32& dst, int32 newVal, Order::t = Order::seqCst)  { return InterlockedExchange(reinterpret_cast<volatile LONG*>(&dst), newVal); }
    static int32 inc(volatile int32& val, Order::t = Order::seqCst)                 { return InterlockedIncrement(reinterpret_cast<volatile LONG*>(&val))-1; }
    static int32 dec(volatile int32& val, Order::t = Order::seqCst)                 { return InterlockedDecrement(reinterpret_cast<volatile LONG*>(&val))+1; }

    /// Read is not atomic on 32-bit systems
    static int64 load(const volatile int64& val, Order::t)                          { return _InterlockedCompareExchange64(const_cast<volatile LONGLONG*>(&val), 0, 0); }
    /// Write is not atomic on 32-bit systems
    static void store(volatile int64& dst, int64 newVal, Order::t o)                { int64 v; do { v = dst; } while (!cas(dst, newVal, v, o)); }
    static bool cas(volatile int64& dst, int64 newVal, int64 cmp, Order::t)         { return _InterlockedCompareExchange64(static_cast<volatile LONGLONG*>(&dst), newVal, cmp) == cmp; }

    static void fence(Order::t o)
    {
        switch (o)
        {
        case Order::consume:
        case Order::acquire:
            _ReadBarrier();             //Prevent compiler re-ordering
            break;
        case Order::release:
            _WriteBarrier();            //Prevent compiler re-ordering
            break;
        case Order::acqRel:
            _ReadWriteBarrier();        //Prevent compiler re-ordering
            break;
        case Order::seqCst:
            MemoryBarrier();            //Prevent compiler/hardware re-ordering
            break;
        }
    }
};

} } }
/** \endcond */