// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Thread/Lock/platform/Mutex.h"

namespace honey
{

template<class Lockable> class UniqueLock;

/// A thread lock where the lock is acquired by suspending thread execution until it becomes available.
/**
  * Use when threads are expected to acquire the lock for a long time before releasing. \n
  * The lock is non-recursive: a thread can only acquire the lock once, a second attempt without unlocking first will deadlock.
  */
class Mutex : platform::Mutex
{
    typedef platform::Mutex Super;
public:
    typedef Super::Handle Handle;
    typedef UniqueLock<Mutex> Scoped;

    Mutex() = default;
    /// Can't copy, silently inits to default
    Mutex(const Mutex&) {}

    /// Can't copy, silently does nothing
    Mutex& operator=(const Mutex&)                  { return *this; }

    /// Acquire the lock.  Thread suspends until lock becomes available.
    void lock()                                     { Super::lock(); }
    /// Release the lock.
    void unlock()                                   { Super::unlock(); }
    /// Attempt to acquire the lock, returns immediately.  Returns true if the lock was acquired, false otherwise.
    bool tryLock()                                  { return Super::tryLock(); }

    /// Get platform handle
    Handle& handle()                                { return Super::handle(); }
};

}
