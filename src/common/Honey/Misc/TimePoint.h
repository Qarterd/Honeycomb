// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Duration.h"

namespace honey
{

/// TimePoint represented by a duration since a clock's epoch time
template<class Clock_, class Dur = typename Clock_::Duration>
class TimePoint
{
    friend class TimePoint;
public:
    typedef Clock_ Clock;
    typedef Dur Duration;

    /// Initialized to zero time, the clock's epoch
    TimePoint()                                                 : _dur(Duration::zero) {}
    /// Construct with a duration from clock's epoch
    explicit TimePoint(const Duration& d)                       : _dur(d) {}
    /// Construct from a time point with the same clock but different duration
    template<class Dur2>
    TimePoint(const TimePoint<Clock, Dur2>& t)                  { operator=(t); }
    /// Construct from a different clock
    template<class Clock2, class Dur2>
    TimePoint(const TimePoint<Clock2, Dur2>& t)                 { operator=(t); }

    template<class Dur2>
    TimePoint& operator=(const TimePoint<Clock, Dur2>& rhs)     { _dur = rhs._dur; return *this; }

    template<class Clock2, class Dur2>
    TimePoint& operator=(const TimePoint<Clock2, Dur2>& rhs)
    {
        _dur = Clock::now() + (rhs._dur - Clock2::now());
        return *this;
    }

    TimePoint& operator+=(const Duration& rhs)                  { _dur += rhs; return *this; }
    TimePoint& operator-=(const Duration& rhs)                  { _dur -= rhs; return *this; }

    template<class Dur2>
    bool operator==(const TimePoint<Clock,Dur2>& rhs) const     { return _dur == rhs._dur; }

    template<class Dur2>
    bool operator!=(const TimePoint<Clock,Dur2>& rhs) const     { return !(*this == rhs); }

    template<class Dur2>
    bool operator<(const TimePoint<Clock,Dur2>& rhs) const      { return _dur < rhs._dur; }

    template<class Dur2>
    bool operator>(const TimePoint<Clock,Dur2>& rhs) const      { return rhs < *this; }

    template<class Dur2>
    bool operator<=(const TimePoint<Clock,Dur2>& rhs) const     { return !(rhs < *this); }
    
    template<class Dur2>
    bool operator>=(const TimePoint<Clock,Dur2>& rhs) const     { return !(*this < rhs); }

    /// Get duration since clock epoch time
    const Duration& time() const                                { return _dur; }

private:
    Duration _dur;

public:
    /// Minimum time point (negative duration)
    static const TimePoint min;
    /// Maximum time point (positive duration)
    static const TimePoint max;
};

/// operator+(TimePoint, Duration).  Returns a time point with a best-fit duration.
/** \relates TimePoint */
template<class Clock, class Dur, class Rep, class Period>
TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>
operator+(const TimePoint<Clock,Dur>& lhs, const Duration<Rep,Period>& rhs)
{
    typedef TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type> TimeCommon;
    return TimeCommon(lhs) += rhs;
}

/// operator+(Duration, TimePoint).  Returns a time point with a best-fit duration.
/** \relates TimePoint */
template<class Clock, class Dur, class Rep, class Period>
TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>
operator+(const Duration<Rep,Period>& rhs, const TimePoint<Clock,Dur>& lhs)
{
    return rhs + lhs;
}

/// operator-(TimePoint, Duration).  Returns a time point with a best-fit duration.
/** \relates TimePoint */
template<class Clock, class Dur, class Rep, class Period>
TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>
operator-(const TimePoint<Clock,Dur>& lhs, const Duration<Rep,Period>& rhs)
{
    return lhs + (-rhs);
}

/// operator-(TimePoint, TimePoint).  Returns a best-fit duration.
/** \relates TimePoint */
template<class Clock, class Dur, class Rep, class Period>
typename Duration<Rep,Period>::template common<Dur>::type
operator-(const TimePoint<Clock,Dur>& lhs, const TimePoint<Clock,Duration<Rep,Period>>& rhs)
{
    return lhs.time() - rhs.time();
}

template<class Clock, class Dur>
const TimePoint<Clock, Dur> TimePoint<Clock, Dur>::min(numeral<typename Dur::Rep>().min());

template<class Clock, class Dur>
const TimePoint<Clock, Dur> TimePoint<Clock, Dur>::max(numeral<typename Dur::Rep>().max());

}

