// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Enum.h"
#include "Honey/Misc/ScopeGuard.h"
#include "Honey/Chrono/Clock.h"
#include "Honey/Thread/Lock/Mutex.h"

namespace honey
{

template<class Lockable> class SharedLock;

namespace lock
{
    enum class Op
    {
        lock,       ///< Lock (blocking)
        tryLock,    ///< Try to lock (non-blocking)
        adopt,      ///< Already locked
        defer       ///< Not yet locked, will lock manually
    };

    /// \name ScopeGuard functors
    /// @{
    template<class Lock>
    function<void ()> lockGuard(Lock& lock)                     { return [&] { lock.lock(); }; }
    template<class Lock>
    function<void ()> unlockGuard(Lock& lock)                   { return [&] { lock.unlock(); }; }
    template<class Lock>
    function<void ()> releaseGuard(Lock& lock)                  { return [&] { lock.release(); }; }
    /// @}
}

/// A scoped lock that references any lockable. Locks on construction and unlocks on destruction.
/**
  * Instances are non-recursive (can't lock an instance twice), and can only be manipulated by one thread. \n
  * Note that if recursion is required, multiple instances can reference the same recursive lockable.
  */
template<class Lockable_>
class UniqueLock : mt::NoCopy
{
public:
    typedef Lockable_ Lockable;

    UniqueLock()                                                : _lock(nullptr), _owns(false) {}

    /// Construct with a reference to a mutex and an operation to perform on construction.
    UniqueLock(Lockable& lock, lock::Op op = lock::Op::lock)    : _lock(&lock), _owns(false)
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
    UniqueLock(Lockable& lock, Duration<Rep,Period> time)       : _lock(&lock), _owns(false) { tryLock(time); }
    template<class Clock, class Dur>
    UniqueLock(Lockable& lock, TimePoint<Clock,Dur> time)       : _lock(&lock), _owns(false) { tryLock(time); }
    
    UniqueLock(UniqueLock&& rhs) noexcept                       { operator=(move(rhs)); }
    /// Unlock shared lock (reader) and block until unique lock (writer) is acquired.  The shared lock is released.
    UniqueLock(SharedLock<Lockable>&& rhs)                      { operator=(move(rhs)); }

    /// Unlock the mutex if we own it
    ~UniqueLock()                                               { if (_owns) unlock(); }

    UniqueLock& operator=(UniqueLock&& rhs)
    {
        _lock = rhs._lock; _owns = rhs._owns;
        rhs._lock = nullptr; rhs._owns = false;
        return *this;
    }

    UniqueLock& operator=(SharedLock<Lockable>&& rhs)
    {
        _lock = &rhs.mutex(); _owns = true;
        _lock->unlockShared();
        _lock->lock();
        rhs.release();
        return *this;
    }

    void lock()
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        _lock->lock();
        _owns = true;
    }

    void unlock()
    {
        assert(_lock, "Lock has been released");
        assert(_owns, "Lock not held");
        _lock->unlock();
        _owns = false;
    }

    bool tryLock()
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLock();
    }

    template<class Rep, class Period>
    bool tryLock(Duration<Rep,Period> time)
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLock(time);
    }

    template<class Clock, class Dur>
    bool tryLock(TimePoint<Clock,Dur> time)
    {
        assert(_lock, "Lock has been released");
        assert(!_owns, "Lock already held");
        return _owns = _lock->tryLock(time);
    }

    /// Check if mutex is locked by this instance
    bool owns() const                                           { return _owns; }
    /// Same as owns()
    explicit operator bool() const                              { return owns(); }
    
    /// Get the referenced mutex
    Lockable& mutex()
    {
        assert(_lock, "Lock has been released");
        return *_lock;
    }

    /// Release the mutex from further operations.  The mutex will no longer be owned and its state will remain unchanged.
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
