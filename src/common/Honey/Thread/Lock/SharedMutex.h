// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Condition/Lock.h"

namespace honey
{

template<class Lockable> class SharedLock;

/// A thread lock for shared data where there may be many readers and one writer.
/**
  * A thread acquiring a writer lock will suspend execution while there are readers. \n
  * The lock is recursive: one thread can acquire the lock multiple times, which must be followed by the same number of unlocks. \n
  * This class uses atomics, so locking without contention is faster than a platform Mutex.
  */
class SharedMutex
{
public:
    typedef UniqueLock<SharedMutex> Scoped;
    typedef SharedLock<SharedMutex> SharedScoped;

    SharedMutex()                                           { init(); }
    /// Can't copy, silently inits to default
    SharedMutex(const SharedMutex&)                         { init(); }

    ~SharedMutex() {}

    /// Can't copy, silently does nothing
    SharedMutex& operator=(const SharedMutex&)              { return *this; }

    /// Acquire the unique writer lock.  Thread suspends until lock is available and all readers release the shared lock.
    void lock();
    /// Release the unique writer lock.
    void unlock();

    /// Attempt to acquire the unique writer lock, returns immediately.  Returns true if the lock was acquired, false otherwise.
    bool tryLock();
    /// Attempt to acquire the unique writer lock for an amount of time.  Returns true if the lock was acquired, false if timed out.
    bool tryLock(MonoClock::Duration time)                  { return tryLock(time == time.max ? MonoClock::TimePoint::max : MonoClock::now() + time); }
    /// Attempt to acquire the unique writer lock until a certain time.  Returns true if the lock was acquired, false if timed out.
    bool tryLock(MonoClock::TimePoint time);

    /// Acquire the shared reader lock.  Thread suspends until writer lock has been released.
    void lockShared();
    /// Release the shared reader lock.
    void unlockShared();

    /// Attempt to acquire the shared reader lock, returns immediately.  Returns true if the lock was acquired, false otherwise.
    bool tryLockShared();
    /// Attempt to acquire the shared reader lock for an amount of time.  Returns true if the lock was acquired, false if timed out.
    bool tryLockShared(MonoClock::Duration time)            { return tryLockShared(time == time.max ? MonoClock::TimePoint::max : MonoClock::now() + time); }
    /// Attempt to acquire the shared reader lock until a certain time.  Returns true if the lock was acquired, false if timed out.
    bool tryLockShared(MonoClock::TimePoint time);

    /// Atomically unlock unique writer lock and acquire shared reader lock without blocking
    void unlockAndLockShared();

private:
    void init();

    /// Lock state: either unlocked = 0, unique lock = 1, or a multiple of 2 for each shared lock
    struct State { enum t {
        unlock  = 0,
        unique  = 1,
        shared  = 2
    }; };

    bool tryLock_priv(Thread::ThreadId threadId);
    bool tryLockShared_priv();

    bool isShared() const                                   { return (_state & ~State::unique) != 0; }

    atomic::Var<int>                _state;
    ConditionLock                   _cond;
    atomic::Var<Thread::ThreadId>   _owner;
    int                             _holdCount;
    atomic::Var<int>                _waitCount;
};


}
