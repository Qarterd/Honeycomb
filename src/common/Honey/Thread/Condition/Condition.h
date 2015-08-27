// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Lock/Unique.h"
#include "Honey/Thread/Condition/platform/Condition.h"

namespace honey
{

/// Method to synchronize threads.  Condition variables eliminate the need for repeated polling to check the value of some data.
/**
  * The mutex must be locked before calling any methods on the condition.
  *
  * This is the default condition class that thinly wraps a platform condition.
  * The default condition is fast but can only use a basic mutex.
  */
class Condition : platform::Condition, mt::NoCopy
{
    typedef platform::Condition Super;
public:
    /// Signal one waiting thread to resume, resumed thread attempts to acquire the lock.
    void signal()                                                       { Super::signal(); }
    /// Signal all waiting threads to resume, all resumed threads attempt to acquire the lock.
    void broadcast()                                                    { Super::broadcast(); }

    /// Release lock and wait until thread is signaled
    void wait(UniqueLock<Mutex>& lock)                                  { wait(lock, MonoClock::TimePoint::max); }
    /// Release lock and wait until thread is signaled or until an amount of time has passed. Returns true if signaled, false if timed out.
    template<class Rep, class Period>
    bool wait(UniqueLock<Mutex>& lock, Duration<Rep,Period> time)       { return wait(lock, time == time.max ? MonoClock::TimePoint::max : MonoClock::now() + time); }
    /// Release lock and wait until thread is signaled or until a certain time. Returns true if signaled, false if timed out.
    template<class Clock, class Dur>
    bool wait(UniqueLock<Mutex>& lock, TimePoint<Clock,Dur> time)
    {
        thread::priv::InterruptWait _(Thread::current(), *this, lock.mutex());
        thread::current::interruptPoint();
        bool res = Super::wait(lock, time);
        thread::current::interruptPoint();
        return res;
    }
};

}
