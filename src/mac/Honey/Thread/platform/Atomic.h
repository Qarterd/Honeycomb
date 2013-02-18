// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include <atomic>

/** \cond */
namespace honey { namespace atomic { namespace platform
{

class Op
{
public:
    static int32 load(volatile const int32& val, Order o)
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
                std::atomic_thread_fence(std::memory_order_acquire);    //Prevent compiler moving operations from after acquire to before
                return val;
            }
        case Order::seqCst:
        default:
            std::atomic_thread_fence(std::memory_order_seq_cst);        //Prevent compiler/hardware re-ordering
            return val;
        }
    }
    
    static void store(volatile int32& dst, int32 newVal, Order o)
    {
        switch (o)
        {
        case Order::relaxed:
            dst = newVal;              //Write is atomic if val is 32-bit aligned
            break;
        case Order::release:
            std::atomic_thread_fence(std::memory_order_release);        //Prevent compiler moving operations from before release to after
            dst = newVal;
            break;
        case Order::seqCst:
        default:
            swap(dst, newVal);         //Swap provides memory barrier
            break;
        }
    }

    static bool cas(volatile int32& dst, int32 newVal, int32 cmp, Order)            { return __sync_bool_compare_and_swap(&dst, cmp, newVal); }
    static int32 swap(volatile int32& dst, int32 newVal, Order o = Order::seqCst)   { int32 v; do { v = dst; } while (!cas(dst, newVal, v, o)); return v; }
    static int32 inc(volatile int32& val, Order = Order::seqCst)                    { return __sync_fetch_and_add(&val, 1); }
    static int32 dec(volatile int32& val, Order = Order::seqCst)                    { return __sync_fetch_and_sub(&val, 1); }
    
    /// Read is not atomic on 32-bit systems
    static int64 load(const volatile int64& val, Order)                             { return __sync_val_compare_and_swap(&val, 0, 0); }
    /// Write is not atomic on 32-bit systems
    static void store(volatile int64& dst, int64 newVal, Order o)                   { int64 v; do { v = dst; } while (!cas(dst, newVal, v, o)); }
    static bool cas(volatile int64& dst, int64 newVal, int64 cmp, Order)            { return __sync_bool_compare_and_swap(&dst, cmp, newVal); }

    static void fence(Order o)                                                      { std::atomic_thread_fence(static_cast<std::memory_order>(o)); }
};

} } }
/** \endcond */