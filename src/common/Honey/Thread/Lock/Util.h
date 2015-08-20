// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Range.h"
#include "Honey/Thread/Lock/Unique.h"

namespace honey
{
/// Mutex lock util
namespace lock
{

//====================================================
// tryLock
//====================================================

/** \cond */
inline int tryLock()    { return -1; }
/** \endcond */

/// Try to lock all lockables. Locks either all or none.
/**
  * Returns the zero-based index of the first failed lock, or -1 if all locks were successful.
  */
template<class Lock, class... Locks, typename mt::disable_if<mt::isRange<Lock>::value, int>::type=0>
int tryLock(Lock& l, Locks&... ls)
{
    UniqueLock<Lock> lock(l, lock::Op::tryLock);
    if (!lock.owns()) return 0;
    int failed = tryLock(ls...);
    if (failed >= 0) return failed+1;
    lock.release();
    return -1;
}

/// Try to lock all lockables in a range. Locks either all or none.
/**
  * Returns an iterator to the first failed lock, or end if all locks were successful.
  */
template<class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
auto tryLock(Range&& range) -> mt_iterOf(range)
{
    auto& begin = honey::begin(range);
    auto& end = honey::end(range);
    if (begin == end) return end;
    UniqueLock<mt_iterOf(range)::value_type> lock(*begin, lock::Op::tryLock);
    if (!lock.owns()) return begin;
    auto failed = tryLock(honey::range(next(begin),end));
    if (failed == end) lock.release();
    return failed;
}

//====================================================
// lock
//====================================================

/** \cond */
namespace priv
{
    /// Part of lock implementation. Locks first mutex then tries to lock rest, returns failed index or -1 on success.
    template<class Lock, class... Locks, typename mt::disable_if<mt::isRange<Lock>::value, int>::type=0>
    int lockTest(Lock& l, Locks&... ls)
    {
        UniqueLock<Lock> lock(l);
        int failed = tryLock(ls...);
        if (failed >= 0) return failed+1;
        lock.release();
        return -1;
    }
    
    /// Part of lock implementation. Locks first mutex then tries to lock rest, returns failed iterator or end on success.
    template<class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
    auto lockTest(Range&& range) -> mt_iterOf(range)
    {
        auto& begin = honey::begin(range);
        auto& end = honey::end(range);
        if (begin == end) return end;
        UniqueLock<mt_iterOf(range)::value_type> lock(*begin);
        auto failed = tryLock(honey::range(next(begin),end));
        if (failed == end) lock.release();
        return failed;
    }

    template<class Locks, szt... Seq>
    void lock(Locks&& locks, mt::idxseq<Seq...>)
    {
        auto switch_ = mt::make_array<function<int ()>>([&]() -> int
        {
            const int offset = Seq;
            int failed;
            if ((failed = priv::lockTest(get<(offset+Seq) % sizeof...(Seq)>(locks)...)) == -1) return -1;
            return (offset+failed) % sizeof...(Seq);
        }...);
        int lockFirst = 0;
        while(lockFirst >= 0) lockFirst = switch_[lockFirst]();
    }
}
/** \endcond */

/// Lock all lockables safely without deadlocking.
/** 
  * Deadlock can be avoided by waiting only for the first lock, then trying to lock the others without waiting.
  * If any of the others fail, restart and wait for a failed lock instead.
  * For example:
  *
  *     Lock L1 and then call:                          tryLock(L2,L3,L4,L5)
  *     If L2 failed then restart, lock L2 and call:    tryLock(L3,L4,L5,L1)
  */
template<class... Locks, typename mt::disable_if<mt::isRange<mt::typeAt<0, Locks...>>::value, int>::type=0>
void lock(Locks&&... locks)                     { priv::lock(forward_as_tuple(forward<Locks>(locks)...), mt::make_idxseq<sizeof...(Locks)>()); }

/// Lock all lockables in a range safely without deadlocking.
template<class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
void lock(Range&& range)
{
    auto lockFirst = begin(range);
    while(true)
    {
        auto it = ringRange(range,lockFirst);
        if ((lockFirst = priv::lockTest(it)) == end(it).iter()) break;
    }
}

} }
