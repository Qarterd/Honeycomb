// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Thread/Condition/Condition.h"
#include "Honey/Memory/UniquePtr.h"
#include "Honey/Thread/Lock/Spin.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey { namespace platform
{

Condition::Condition() :
    _waitCount(0),
    _waitLock(new SpinLock),
    _broadcast(false)
{
    _sema = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    assert(_sema != NULL);

    _waitDone = CreateEvent(NULL,FALSE,FALSE,NULL);
    assert(_waitDone != NULL);

    _timer = CreateWaitableTimer(NULL, FALSE, NULL);
    assert(_timer != NULL);
}

Condition::~Condition()
{
    verify(CloseHandle(_sema));
    verify(CloseHandle(_waitDone));
    verify(CloseHandle(_timer));
}

void Condition::signal()
{
    _waitLock->lock();
    bool haveWait = _waitCount > 0;
    _waitLock->unlock();

    if (haveWait)
        ReleaseSemaphore(_sema, 1, NULL);
}

void Condition::broadcast()
{
    //If two threads simultaneously broadcast this method can deadlock.
    //An external lock must be held before calling this method.
    SpinLock::Scoped lock(*_waitLock);

    if (_waitCount > 0)
    {
        //Wake up all the waiters
        _broadcast = true;
        ReleaseSemaphore(_sema, _waitCount, NULL);
        lock.unlock();
        //To be fair, wait until all the waiters have woken up.
        WaitForSingleObject(_waitDone, INFINITE);
        _broadcast = false;
    }
}

bool Condition::wait(UniqueLock<honey::Mutex>& external, honey::MonoClock::TimePoint time)
{
    honey::thread::priv::InterruptWait _(static_cast<honey::Condition&>(*this), external.mutex());
    auto __ = ScopeGuard(lock::lockGuard(external));

    _waitLock->lock();
    ++_waitCount;
    _waitLock->unlock();

    //Wait for both the semaphore and the high resolution timeout
    HANDLE handles[2] = { _sema };
    int handleCount = 1;
    if (time != honey::MonoClock::TimePoint::max)
    {
        //Convert to windows 100 nanosecond period, negative time means relative
        LARGE_INTEGER sleepTime;
        sleepTime.QuadPart = -Alge::max(time - honey::MonoClock::now(), honey::MonoClock::Duration::zero) / 100;
        verify(SetWaitableTimer(_timer, &sleepTime, 0, NULL, NULL, 0));
        handles[handleCount++] = _timer;
    }

    //Unfair but safe race condition: external unlock and wait should be atomic
    external.unlock();
    DWORD res = WaitForMultipleObjects(handleCount, handles, FALSE, INFINITE);

    _waitLock->lock();
    --_waitCount;
    bool lastWait = _broadcast && _waitCount == 0;
    _waitLock->unlock();

    //Unfair but safe race condition: wait done signal and external relock should be atomic
    if (lastWait)
        SetEvent(_waitDone);
    return res == WAIT_OBJECT_0;
} //external.lock(); ~InterruptWait()

} }
/** \endcond */



