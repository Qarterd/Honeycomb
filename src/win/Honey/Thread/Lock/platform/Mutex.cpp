// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Thread/Lock/Mutex.h"
#include "Honey/Thread/Condition/Condition.h"
#include "Honey/Thread/Thread.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey { namespace platform
{

Mutex::Mutex(bool timed) :
    _tryWaitCount(0),
    _tryLock(timed ? new honey::Mutex(mt::tag<0>()) : nullptr),
    _tryCond(timed ? new honey::Condition : nullptr)
{
    InitializeCriticalSection(&_handle);
}

Mutex::~Mutex()
{
    DeleteCriticalSection(&_handle);
}

void Mutex::lock()                                          { EnterCriticalSection(&_handle); }

void Mutex::unlock()
{
    bool unlocked = _handle.RecursionCount <= 1;
    LeaveCriticalSection(&_handle);
    if (!unlocked) return;
    //Notify try waiters that lock has been released
    if (_tryWaitCount == 0) return;
    UniqueLock<honey::Mutex> _(*_tryLock); 
    _tryCond->signal();
}

bool Mutex::tryLock()                                       { return TryEnterCriticalSection(&_handle); }  

bool Mutex::tryLock(honey::MonoClock::TimePoint time)
{
    assert(_tryLock, "Unsupported Op: not a timed mutex");
    if (tryLock()) return true;

    UniqueLock<honey::Mutex> lock(*_tryLock);
    bool res;
    ++_tryWaitCount;
    while ((res = tryLock()) == false && _tryCond->wait(lock, time));
    --_tryWaitCount;
    return res;
}

} }
/** \endcond */



