// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Lock/TimedMutex.h"

namespace honey
{

void TimedMutex::unlock()
{
    Mutex::unlock();
    //Notify try waiters that lock has been released
    if (_tryWaitCount == 0) return;
    ConditionLock::Scoped _(_tryCond);
    _tryCond.signal();
}

bool TimedMutex::tryLock(honey::MonoClock::TimePoint time)
{
    ConditionLock::Scoped _(_tryCond);
    bool res;
    ++_tryWaitCount;
    while (!(res = tryLock()) && _tryCond.wait(time));
    --_tryWaitCount;
    return res;
}

}
