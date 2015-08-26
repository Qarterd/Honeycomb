// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Lock/Shared.h"

namespace honey
{

/// Scoped transfer of mutex ownership between two locks.
/**
  * Ownership is transferred to `ToLock` on construction, and then returned to `FromLock` on destruction.
  *
  * This class can be used to atomically acquire a unique writer lock from a shared reader lock:
  *
  *     SharedMutex mutex;
  *     SharedMutex::SharedScoped readLock(mutex);
  *     //Read ownership acquired, read data...
  *     {
  *         TransferLock<decltype(readLock), SharedMutex::Scoped> writeLock(readLock);
  *         //Write ownership acquired in this scope, write data...
  *     }
  *     //Read ownership re-acquired
  */
template<class FromLock, class ToLock>
class TransferLock : mt::NoCopy
{
public:
    typedef typename ToLock::Lockable Lockable;

    /// Transfer ownership of mutex from `lock` to an instance of ToLock
    TransferLock(FromLock& lock)                            : _lock(&lock) { this->lock(); }

    ~TransferLock()                                         { if (owns()) unlock(); }

    void lock()
    {
        assert(_lock, "Lock has been released");
        assert(!owns(), "Lock already held");
        _toLock = move(*_lock);
    }

    void unlock()
    {
        assert(_lock, "Lock has been released");
        assert(owns(), "Lock not held");
        *_lock = move(_toLock);
    }

    bool owns() const                                       { return _lock && _toLock.owns(); }
    explicit operator bool() const                          { return owns(); }
    
    Lockable& mutex()
    {
        assert(_lock, "Lock has been released");
        return _toLock.mutex();
    }

    Lockable& release()
    {
        Lockable& ret = mutex();
        _lock = nullptr;
        return ret;
    }

private:
    FromLock* _lock;
    ToLock _toLock;
};

}
