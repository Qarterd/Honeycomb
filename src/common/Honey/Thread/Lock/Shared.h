// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Lock/SharedMutex.h"

namespace honey
{

/// A scoped lock that references a shared mutex. Does a shared read lock on construction and unlocks on destruction.
/**
  * Instances are non-recursive (can't lock an instance twice), and can only be manipulated by one thread. \n
  * Note that if recursion is required, multiple instances can reference the same recursive lockable.
  */
template<class Lockable_>
class SharedLock : mt::NoCopy
{
public:
    typedef Lockable_ Lockable;

    SharedLock()                                                : _lock(nullptr), _owns(false) {}

    SharedLock(Lockable& lock, lock::Op op = lock::Op::lock)    : _lock(&lock), _owns(false)
    {
        switch (op)
        {
        case lock::Op::lock:
            this->lock();
            break;
        case lock::Op::tryLock:
            tryLock();
            break;
        case lock::Op::adopt:
            _owns = true;
            break;
        case lock::Op::defer:
            break;
        }
    }

    template<class Rep, class Period>
    SharedLock(Lockable& lock, Duration<Rep,Period> time)       : _lock(&lock), _owns(false) { tryLock(time); }
    template<class Clock, class Dur>
    SharedLock(Lockable& lock, TimePoint<Clock,Dur> time)       : _lock(&lock), _owns(false) { tryLock(time); }

    SharedLock(SharedLock&& rhs) noexcept                       { operator=(move(rhs)); }
    /// Atomically unlock unique writer lock and acquire shared reader lock without blocking. The unique lock is released.
    SharedLock(UniqueLock<Lockable>&& rhs)                      { operator=(move(rhs)); }

    ~SharedLock()                                               { if (_owns) unlock(); }

    SharedLock& operator=(SharedLock&& rhs)
    {
        _lock = rhs._lock; _owns = rhs._owns;
        rhs._lock = nullptr; rhs._owns = false;
        return *this;
    }

    SharedLock& operator=(UniqueLock<Lockable>&& rhs)
    {
        _lock = &rhs.mutex(); _owns = true;
        _lock->unlockAndLockShared();
        rhs.release();
        return *this;
    }

    void lock()
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        _lock->lockShared();
        _owns = true;
    }

    void unlock()
    {
        assert(_lock, "Lock has been released");
        assert(_owns, "Lock not held");
        _lock->unlockShared();
        _owns = false;
    }

    bool tryLock()
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLockShared();
    }

    template<class Rep, class Period>
    bool tryLock(Duration<Rep,Period> time)
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLockShared(time);
    }

    template<class Clock, class Dur>
    bool tryLock(TimePoint<Clock,Dur> time)
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLockShared(time);
    }

    bool owns() const                                           { return _owns; }
    explicit operator bool() const                              { return owns(); }
    
    Lockable& mutex()
    {
        assert(_lock, "Lock has been released");
        return *_lock;
    }

    Lockable& release()
    {
        Lockable& ret = mutex();
        _lock = nullptr;
        _owns = false;
        return ret;
    }

private:
    Lockable* _lock;
    bool _owns;
};


}
