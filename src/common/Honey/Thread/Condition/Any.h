// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Condition/Condition.h"

namespace honey
{

/// Condition that can be used with any kind of lock.  Slightly slower than the default Condition class.
class ConditionAny
{
public:

    void signal()                                                       { Mutex::Scoped _(_lock); _cond.signal(); }

    void broadcast()                                                    { Mutex::Scoped _(_lock); _cond.broadcast(); }

    template<class Lockable>
    void wait(Lockable& lock)                                           { wait(lock, MonoClock::TimePoint::max); }

    template<class Lockable, class Rep, class Period>
    bool wait(Lockable& lock, Duration<Rep,Period> time)                { return wait(lock, MonoClock::now() + time); }

    template<class Lockable, class Clock, class Dur>
    bool wait(Lockable& external, TimePoint<Clock,Dur> time)
    {
        //Scoping used to ensure state is returned to normal if an exception is thrown
        Mutex::Scoped internal(_lock);
        external.unlock();
        auto _ = ScopeGuard(lock::lockGuard(external));
        UniqueLock<Mutex::Scoped> __(internal, lock::Op::adopt);
        return _cond.wait(internal, time);
    } //internal.unlock(); external.lock();

private:
    Condition _cond;
    Mutex     _lock;
};

}
