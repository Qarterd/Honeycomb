// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/TimePoint.h"
#include "Honey/Misc/platform/Clock.h"

namespace honey
{

/// System-wide real-time clock.  Low-resolution time since Unix Epoch, can possibly go backwards if changed by OS.
struct SystemClock : private platform::SystemClock<SystemClock>
{
private:
    typedef platform::SystemClock<SystemClock>          Super;
public:
    typedef Super::TimePoint                            TimePoint;
    typedef TimePoint::Duration                         Duration;
    typedef Duration::Rep                               Rep;
    typedef Duration::Period                            Period;

    static const bool isMonotonic                       = false;

    /// Get current time
    static TimePoint now()                              { return Super::now(); }

    /// Convert to standard C time
    static time_t toStdTime(const TimePoint& t)         { return time_t(Seconds(t.time())); }

    /// Convert from standard C time
    static TimePoint fromStdTime(const time_t& t)       { return TimePoint(Seconds(t)); }
};


/// System-wide monotonic clock.  High-resolution and steady-rate time since application start, can't go backwards.
struct MonoClock : private platform::MonoClock<MonoClock>
{
private:
    typedef platform::MonoClock<MonoClock>              Super;
public:
    typedef Super::TimePoint                            TimePoint;
    typedef TimePoint::Duration                         Duration;
    typedef Duration::Rep                               Rep;
    typedef Duration::Period                            Period;

    static const bool isMonotonic                       = true;

    /// Get current time
    static TimePoint now()                              { return Super::now(); }
};

}

