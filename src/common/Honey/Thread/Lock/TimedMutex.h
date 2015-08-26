// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Atomic.h"
#include "Honey/Thread/Condition/Lock.h"

namespace honey
{

/// A mutex that has a timed try-lock
class TimedMutex : public Mutex
{
public:
    typedef UniqueLock<TimedMutex> Scoped;

    TimedMutex()                                    : _tryWaitCount(0) {}
    /// Can't copy, silently inits to default
    TimedMutex(const TimedMutex&)                   : TimedMutex() {}

    /// Can't copy, silently does nothing
    TimedMutex& operator=(const TimedMutex&)        { return *this; }

    /// Acquire the lock.  Thread suspends until lock becomes available.
    void lock()                                     { Mutex::lock(); }
    /// Release the lock.
    void unlock();
    /// Attempt to acquire the lock, returns immediately.  Returns true if the lock was acquired, false otherwise.
    bool tryLock()                                  { return Mutex::tryLock(); }
    /// Attempt to acquire the lock for an amount of time.  Returns true if the lock was acquired, false if timed out.
    bool tryLock(MonoClock::Duration time)          { return tryLock(time == time.max ? MonoClock::TimePoint::max : MonoClock::now() + time); }
    /// Attempt to acquire the lock until a certain time.  Returns true if the lock was acquired, false if timed out.
    bool tryLock(MonoClock::TimePoint time);

private:
    atomic::Var<int> _tryWaitCount;
    ConditionLock _tryCond;
};

}
