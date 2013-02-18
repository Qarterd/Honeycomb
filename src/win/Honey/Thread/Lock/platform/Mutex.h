// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Atomic.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{

class Condition;
class Mutex;

/** \cond */
namespace platform
{

/// Provides timed mutex functionality to a windows critical section
class Mutex
{
public:
    typedef CRITICAL_SECTION Handle;

    Mutex(bool timed);
    virtual ~Mutex();

    void lock();
    void unlock();
    bool tryLock();
    bool tryLock(honey::MonoClock::Duration time)       { return tryLock(honey::MonoClock::now() + time); }
    bool tryLock(honey::MonoClock::TimePoint time);
    Handle& handle()                                    { return _handle; }

private:
    Handle                          _handle;
    atomic::Var<int>                _tryWaitCount;
    UniquePtr<honey::Mutex>         _tryLock;
    UniquePtr<honey::Condition>     _tryCond;
};

} }
/** \endcond */
