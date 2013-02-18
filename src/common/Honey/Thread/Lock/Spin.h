// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Lock/Unique.h"
#include "Honey/Thread/Atomic.h"
#include "Honey/Thread/Thread.h"

namespace honey
{

/// A thread lock where the lock is acquired through a busy wait loop.
/**
  * Use when threads are expected to quickly acquire and release the lock. \n
  * The lock is recursive: one thread can acquire the lock multiple times, which must be followed by the same number of unlocks. \n
  * This class uses atomics, so locking without contention is faster than a platform Mutex.
  */
class SpinLock
{
public:
    typedef UniqueLock<SpinLock> Scoped;

    SpinLock()                                      : _owner(Thread::threadIdInvalid), _holdCount(0) {}
    /// Can't copy, silently inits to default
    SpinLock(const SpinLock&)                       : _owner(Thread::threadIdInvalid), _holdCount(0) {}

    ~SpinLock()                                     {}

    /// Can't copy, silently does nothing
    SpinLock& operator=(const SpinLock&)            { return *this; }

    /// Acquire the lock.  Thread waits in a busy loop until lock becomes available.
    void lock();
    /// Release the lock.
    void unlock();

    /// Attempt to acquire the lock, returns immediately.  Returns true if the lock was acquired, false otherwise.
    bool tryLock();
    /// Attempt to acquire the lock for an amount of time.  Returns true if the lock was acquired, false otherwise.
    bool tryLock(MonoClock::Duration time)          { return tryLock(MonoClock::now() + time); }
    /// Attempt to acquire the lock until a certain time.  Returns true if the lock was acquired, false otherwise.
    bool tryLock(MonoClock::TimePoint time);

private:
    bool tryLock_priv(Thread::ThreadId threadId);

    atomic::Var<Thread::ThreadId> _owner;
    int _holdCount;
};

}
