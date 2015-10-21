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
    constexpr TimePoint()                                       : _dur(Duration::zero()) {}
    /// Construct with a duration from clock's epoch
    constexpr explicit TimePoint(const Duration& d)             : _dur(d) {}
    /// Construct from a time point with the same clock but different duration
    template<class Dur2>
    constexpr TimePoint(const TimePoint<Clock, Dur2>& t)        : _dur(t._dur) {}
    /// Construct from a different clock
    template<class Clock2, class Dur2>
    TimePoint(const TimePoint<Clock2, Dur2>& t)                 { operator=(t); }

    template<class Dur2>
    TimePoint& operator=(const TimePoint<Clock, Dur2>& rhs)     { _dur = rhs._dur; return *this; }
    template<class Clock2, class Dur2>
    TimePoint& operator=(const TimePoint<Clock2, Dur2>& rhs)    { _dur = Clock::now() + (rhs._dur - Clock2::now()); return *this; }

    TimePoint& operator+=(const Duration& rhs)                  { _dur += rhs; return *this; }
    TimePoint& operator-=(const Duration& rhs)                  { _dur -= rhs; return *this; }

    template<class Dur2>
    constexpr bool operator==(const TimePoint<Clock,Dur2>& rhs) const   { return _dur == rhs._dur; }

    template<class Dur2>
    constexpr bool operator!=(const TimePoint<Clock,Dur2>& rhs) const   { return !(*this == rhs); }

    template<class Dur2>
    constexpr bool operator<(const TimePoint<Clock,Dur2>& rhs) const    { return _dur < rhs._dur; }

    template<class Dur2>
    constexpr bool operator>(const TimePoint<Clock,Dur2>& rhs) const    { return rhs < *this; }

    template<class Dur2>
    constexpr bool operator<=(const TimePoint<Clock,Dur2>& rhs) const   { return !(rhs < *this); }
    
    template<class Dur2>
    constexpr bool operator>=(const TimePoint<Clock,Dur2>& rhs) const   { return !(*this < rhs); }

    /// Get duration since clock epoch time
    constexpr Duration time() const                             { return _dur; }

    /// Minimum time point (negative duration)
    static constexpr TimePoint min()                            { return TimePoint(Dur(numeral<typename Dur::Rep>().min())); }
    /// Maximum time point (positive duration)
    static constexpr TimePoint max()                            { return TimePoint(Dur(numeral<typename Dur::Rep>().max())); }
    
private:
    Duration _dur;
};

/// \name TimePoint operators
/// \relates TimePoint
/// @{

/// operator+(TimePoint, Duration). Returns a time point of best-fit duration.
template<class Clock, class Dur, class Rep, class Period,
    class TimeCommon = TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>>
constexpr TimeCommon operator+(const TimePoint<Clock,Dur>& lhs, const Duration<Rep,Period>& rhs)
{
    return TimeCommon(lhs.time() + rhs);
}
/// operator+(Duration, TimePoint). Returns a time point of best-fit duration.
template<class Clock, class Dur, class Rep, class Period,
    class TimeCommon = TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>>
constexpr TimeCommon operator+(const Duration<Rep,Period>& rhs, const TimePoint<Clock,Dur>& lhs)
{
    return TimeCommon(rhs + lhs.time());
}
/// operator-(TimePoint, Duration). Returns a time point of best-fit duration.
template<class Clock, class Dur, class Rep, class Period,
    class TimeCommon = TimePoint<Clock, typename Duration<Rep,Period>::template common<Dur>::type>>
constexpr TimeCommon operator-(const TimePoint<Clock,Dur>& lhs, const Duration<Rep,Period>& rhs)
{
    return lhs + (-rhs);
}
/// operator-(TimePoint, TimePoint). Returns a best-fit duration.
template<class Clock, class Dur, class Rep, class Period,
     class DurCommon = typename Duration<Rep,Period>::template common<Dur>::type>
constexpr DurCommon operator-(const TimePoint<Clock,Dur>& lhs, const TimePoint<Clock,Duration<Rep,Period>>& rhs)
{
    return lhs.time() - rhs.time();
}
/// @}

}

