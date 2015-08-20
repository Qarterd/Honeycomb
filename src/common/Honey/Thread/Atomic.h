// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"

namespace honey
{
/// Atomic operations
namespace atomic
{

/// Atomic memory order for concurrent synchronization between threads.
/**
  * Compilers and hardware optimize ops assuming the environment is single-threaded, this causes race conditions in a concurrent environment. \n
  * The safest but slowest order is sequential consistency, load/store ops will not be optimized and thus will be executed in the order as written. \n
  * The fastest but unsafest order is relaxed, load/store ops can be fully optimized and thus re-ordered. \n
  * Release and acquire pairs provide a middle ground that allows some re-ordering. \n
  * A release on an atomic in thread 1 will synchronize with an acquire on that same atomic in thread 2. \n
  * Synchronization guarantees that all operations before the release in thread 1 will be executed before the acquire in thread 2.
  *
  * \see std::memory_order for details.
  */
enum class Order
{
    relaxed,           ///< No order constraint, same as plain load/store. Unsafe but best performance
    consume,           ///< Must be a load op. Synchronize with a prior release in another thread, but only synchronize ops dependent on this load.
    acquire,           ///< Must be a load op. Synchronize with a prior release in another thread.
    release,           ///< Must be a store op. Synchronize with a later acquire in another thread.
    acqRel,            ///< Must be a load-modify-store op. Performs both acquire and release.
    seqCst             ///< Sequential consistency, safe total order but least performance
};

} }

#include "Honey/Thread/platform/Atomic.h"

namespace honey { namespace atomic
{

/// Methods to perform thread-safe atomic read/write operations
class Op : platform::Op
{
    typedef platform::Op Super;
public:

    /// Returns val
    template<class T>
    static T load(volatile const T& val, Order o = Order::seqCst)               { return Super::load(val, o); }
    using Super::load;

    /// Assigns dst to newVal
    template<class T>
    static void store(volatile T& dst, T newVal, Order o = Order::seqCst)       { return Super::store(dst, newVal, o); }
    using Super::store;

    /// Compare and swap.  If dst is equal to comparand `cmp` then dst is assigned to newVal and true is returned.  Returns false otherwise.
    template<class T>
    static bool cas(volatile T& dst, T newVal, T cmp, Order o = Order::seqCst)  { return Super::cas(dst, newVal, cmp, o); }

    /// Assigns dst to newVal and returns initial value of dst.
    template<class T>
    static T swap(volatile T& dst, T newVal, Order o = Order::seqCst)           { T v; do { v = dst; } while (!cas(dst, newVal, v, o)); return v; }
    using Super::swap;
    
    /// Increments val. Returns the initial value.
    template<class T>
    static T inc(volatile T& val, Order o = Order::seqCst)                      { T v; do { v = val; } while (!cas(val, v+1, v, o)); return v; }
    using Super::inc;
    
    /// Decrements val. Returns the initial value.
    template<class T>
    static T dec(volatile T& val, Order o = Order::seqCst)                      { T v; do { v = val; } while (!cas(val, v-1, v, o)); return v; }
    using Super::dec;
    
    /// val += rhs. Returns the initial value.
    template<class T>
    static T add(volatile T& val, T rhs, Order o = Order::seqCst)               { T v; do { v = val; } while (!cas(val, v+rhs, v, o)); return v; }
    
    /// val &= rhs. Returns the initial value.
    template<class T>
    static T and_(volatile T& val, T rhs, Order o = Order::seqCst)              { T v; do { v = val; } while (!cas(val, v&rhs, v, o)); return v; }
    
    /// val |= rhs. Returns the initial value.
    template<class T>
    static T or_(volatile T& val, T rhs, Order o = Order::seqCst)               { T v; do { v = val; } while (!cas(val, v|rhs, v, o)); return v; }
    
    /// val ^= rhs. Returns the initial value.
    template<class T>
    static T xor_(volatile T& val, T rhs, Order o = Order::seqCst)              { T v; do { v = val; } while (!cas(val, v^rhs, v, o)); return v; }

    /// Create a memory barrier that synchronizes operations.
    /**
      * An acquire fence synchronizes with all releases before it. \n
      * A release fence synchronizes with all acquires after it. \n
      * A sequencial fence is a sequentially consistent acquire and release fence.
      *
      * \see std::atomic_thread_fence for details.
      */
    static void fence(Order o = Order::seqCst)                                  { Super::fence(o); }
};

/** \cond */
namespace priv
{
    /// Get an atomic type that is large enough to hold T
    template<class T> struct VarType                            { typedef typename std::conditional<sizeof(T) <= 4, int32, int64>::type type; };
}
/** \endcond */

/// Wrapper around integer types to make all operations atomic.  Default order for read/assign is relaxed for best performance, all other ops are sequential.
template<class T>
class Var
{
    typedef typename priv::VarType<T>::type VarType;
public:
    Var() = default;
    Var(T val)                                                  { operator=(val); }
    Var(const Var& val)                                         { operator=(val); }

    /// Assign value
    T operator=(T val) volatile                                 { store(val); return *this; }
    T operator=(const Var& val) volatile                        { store(val); return *this; }

    /// Pre-increment, returns new value
    T operator++() volatile                                     { return Op::inc(_val) + 1; }
    /// Post-increment, returns initial value
    T operator++(int) volatile                                  { return Op::inc(_val); }
    /// Pre-decrement, returns new value
    T operator--() volatile                                     { return Op::dec(_val) - 1; }
    /// Post-decrement, returns initial value
    T operator--(int) volatile                                  { return Op::dec(_val); }
    /// Add and return new value
    T operator+=(T rhs) volatile                                { return add(rhs); }
    /// Sub and return new value
    T operator-=(T rhs) volatile                                { return sub(rhs); }
    /// And and return new value
    T operator&=(T rhs) volatile                                { return and_(rhs); }
    /// Or and return new value
    T operator|=(T rhs) volatile                                { return or_(rhs); }
    /// Xor and return new value
    T operator^=(T rhs) volatile                                { return xor_(rhs); }

    /// Read value
    operator T() const volatile                                 { return load(); }
   
    void store(T val, Order o = Order::seqCst) volatile         { Op::store(_val, static_cast<VarType>(val), o); }
    T load(Order o = Order::seqCst) const volatile              { return Op::load(_val, o); }
    T add(T rhs, Order o = Order::seqCst) volatile              { return Op::add(_val, static_cast<VarType>(rhs), o) + rhs; }
    T sub(T rhs, Order o = Order::seqCst) volatile              { return Op::add(_val, static_cast<VarType>(-rhs), o) - rhs; }
    T and_(T rhs, Order o = Order::seqCst) volatile             { return Op::and_(_val, static_cast<VarType>(rhs), o) & rhs; }
    T or_(T rhs, Order o = Order::seqCst) volatile              { return Op::or_(_val, static_cast<VarType>(rhs), o) | rhs; }
    T xor_(T rhs, Order o = Order::seqCst) volatile             { return Op::xor_(_val, static_cast<VarType>(rhs), o) ^ rhs; }

    /// Compare and swap.  If atomic is equal to comparand `cmp` then atomic is assigned to newVal and true is returned. Returns false otherwise.
    bool cas(T newVal, T cmp, Order o = Order::seqCst) volatile { return Op::cas(_val, static_cast<VarType>(newVal), static_cast<VarType>(cmp), o); }

private:
    VarType _val;
};

/// Wrapper around pointer types to make all operations atomic.
template<class T>
class Var<T*>
{
    typedef typename priv::VarType<T*>::type VarType;
public:
    Var() = default;
    Var(T* val)                                                 { operator=(val); }
    Var(const Var& val)                                         { operator=(val); }

    T* operator=(T* val) volatile                               { store(val); return *this; }
    T* operator=(const Var& val) volatile                       { store(val); return *this; }

    T* operator++() volatile                                    { return reinterpret_cast<T*>(Op::add(_val, sizeof(T))) + 1; }
    T* operator++(int) volatile                                 { return reinterpret_cast<T*>(Op::add(_val, sizeof(T))); }
    T* operator--() volatile                                    { return reinterpret_cast<T*>(Op::add(_val, -sizeof(T))) - 1; }
    T* operator--(int) volatile                                 { return reinterpret_cast<T*>(Op::add(_val, -sizeof(T))); }
    T* operator+=(sdt rhs) volatile                             { return add(rhs); }
    T* operator-=(sdt rhs) volatile                             { return sub(rhs); }

    T* operator->() const volatile                              { return load(); }
    T& operator*() const volatile                               { return *load(); }
    operator T*() const volatile                                { return load(); }

    void store(T* val, Order o = Order::seqCst) volatile        { Op::store(_val, reinterpret_cast<VarType>(val), o); }
    T* load(Order o = Order::seqCst) const volatile             { return reinterpret_cast<T*>(Op::load(_val, o)); }
    T* add(sdt rhs, Order o = Order::seqCst) volatile           { return reinterpret_cast<T*>(Op::add(_val, rhs*sizeof(T), o)) + rhs; }
    T* sub(sdt rhs, Order o = Order::seqCst) volatile           { return reinterpret_cast<T*>(Op::add(_val, -rhs*sizeof(T), o)) - rhs; }

    bool cas(T* newVal, T* cmp, Order o = Order::seqCst) volatile   { return Op::cas(_val, reinterpret_cast<VarType>(newVal), reinterpret_cast<VarType>(cmp), o); }

private:
    VarType _val;
};

} 

/** \relates atomic::Var */
template<class T>
ostream& operator<<(ostream& os, const atomic::Var<T>& val)     { return os << val.load(); }

}