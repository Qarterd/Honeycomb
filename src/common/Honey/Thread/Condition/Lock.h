// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Condition/Condition.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{

/// Lock that is bound to a single condition.  This is the common usage case of condition variables.
/**
  * Example:
  *
  * \code
  *
  * --Consumer Thread--
  * cond.lock();                         //Lock to protect data access
  * while (data < 10) { cond.wait(); }   //Read data, temporarily release lock while waiting for producer to signal that new data is available
  * cond.unlock();                       //Condition no longer needed
  *
  * --Producer Thread--
  * cond.lock();                         //Lock to protect data access
  * data++;                              //Write data
  * cond.signal();                       //Signal waiting consumer
  * cond.unlock();                       //Allow consumer thread to resume
  *
  * \endcode
  *
  * The condition must be locked before calling any methods on it.
  *
  * Due to "spurious wakeups" (wakeups without a signal), conditions should always wait() in a while loop,
  * and the predicate (ex. data < 10) should always be checked.
  */
class ConditionLock : public Condition, public Mutex
{
public:
    typedef UniqueLock<ConditionLock> Scoped;

    //Undefined
    void wait(UniqueLock<Mutex>& lock);
    template<class Rep, class Period>
    bool wait(UniqueLock<Mutex>& lock, Duration<Rep,Period> time);
    template<class Clock, class Dur>
    bool wait(UniqueLock<Mutex>& lock, TimePoint<Clock,Dur> time);

    /// No lock arg required
    void wait()                                                         { wait(MonoClock::TimePoint::max); }
    template<class Rep, class Period>
    bool wait(Duration<Rep,Period> time)                                { return wait(MonoClock::now() + time); }
    template<class Clock, class Dur>
    bool wait(TimePoint<Clock,Dur> time)
    {
        //Scoping used to ensure state is returned to normal if an exception is thrown
        Mutex::Scoped lock(*this, lock::Op::adopt);
        auto _ = ScopeGuard(lock::releaseGuard(lock));
        return Condition::wait(lock, time);
    } //lock.release();
};

}
