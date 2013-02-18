// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Lock/Spin.h"
#include "Honey/Math/Alge/Alge.h"

namespace honey
{

void SpinLock::lock()
{
    Thread::ThreadId threadId = Thread::current().threadId();
    do
    {
        if (tryLock_priv(threadId)) break;
        thread::current::pause();
    } while (true);
}

void SpinLock::unlock()
{
    assert(_owner == Thread::current().threadId() && _holdCount > 0, "Unlock Error: Thread does not hold the lock.");
    if (--_holdCount > 0) return;
    _owner = Thread::threadIdInvalid;
}

inline bool SpinLock::tryLock_priv(Thread::ThreadId threadId)
{
    if (_owner != threadId && !_owner.cas(threadId, Thread::threadIdInvalid))
        return false;
    ++_holdCount;
    return true;
}

bool SpinLock::tryLock()                                    { return tryLock_priv(Thread::current().threadId()); }

bool SpinLock::tryLock(MonoClock::TimePoint time)
{
    Thread::ThreadId threadId = Thread::current().threadId();
    do
    {
        if (tryLock_priv(threadId)) return true;
        if (MonoClock::now() >= time) break;
        thread::current::pause();
    } while (true);
    return false;
}

}




