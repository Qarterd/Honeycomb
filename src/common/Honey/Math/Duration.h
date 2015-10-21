// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Ratio.h"
#include "Honey/Math/Numeral.h"

namespace honey
{

/// Duration represented by repetitions of a period.  The period must be a ratio.
template<class Rep_, class Period_ = Ratio<1>>
class Duration
{
    template<class Rep, class Period> friend class Duration;
public:
    typedef Rep_ Rep;
    typedef Period_ Period;

    static_assert(Period::num > 0, "Period can't be negative or 0");

    Duration() = default;
    /// Construct from any type of repetition
    template<class Rep2>
    Duration(const Rep2& rep)                                   { operator=(rep); }
    /// Construct from another type of duration
    template<class Rep2, class Period2>
    Duration(const Duration<Rep2,Period2>& d)                   { operator=(d); }

    template<class Rep2>
    Duration& operator=(const Rep2& rhs)                        { _rep = static_cast<Rep>(rhs); return *this; }

    template<class Rep2, class Period2>
    Duration& operator=(const Duration<Rep2,Period2>& rhs)
    {
        typedef typename Period2::template div<Period>::type Ratio;
        _rep = rhs._rep * Ratio::num / Ratio::den;
        return *this;
    }

    Duration operator+() const                                  { return *this; }
    Duration& operator++()                                      { ++_rep; return *this; }
    Duration operator++(int) const                              { return Duration(_rep++); }
    Duration& operator+=(const Duration& rhs)                   { _rep += rhs._rep; return *this; }

    Duration operator-() const                                  { return Duration(-_rep); }
    Duration& operator--()                                      { --_rep; return *this; }
    Duration operator--(int) const                              { return Duration(_rep--); }
    Duration& operator-=(const Duration& rhs)                   { _rep -= rhs._rep; return *this; }

    Duration& operator*=(const Duration& rhs)                   { _rep *= rhs._rep; return *this; }
    Duration& operator/=(const Duration& rhs)                   { _rep /= rhs._rep; return *this; }
    Duration& operator%=(const Duration& rhs)                   { _rep %= rhs._rep; return *this; }

    template<class Rep2, class Period2>
    bool operator==(const Duration<Rep2,Period2>& rhs) const
    {
        typedef typename common<Duration<Rep2,Period2>>::type DurCommon;
        return DurCommon(*this)._rep == DurCommon(rhs)._rep;
    }

    template<class Rep2, class Period2>
    bool operator!=(const Duration<Rep2,Period2>& rhs) const    { return !(*this == rhs); }

    template<class Rep2, class Period2>
    bool operator<(const Duration<Rep2,Period2>& rhs) const
    {
        typedef typename common<Duration<Rep2,Period2>>::type DurCommon;
        return DurCommon(*this)._rep < DurCommon(rhs)._rep;
    }

    template<class Rep2, class Period2>
    bool operator>(const Duration<Rep2,Period2>& rhs) const     { return rhs < *this; }

    template<class Rep2, class Period2>
    bool operator<=(const Duration<Rep2,Period2>& rhs) const    { return !(rhs < *this); }
    
    template<class Rep2, class Period2>
    bool operator>=(const Duration<Rep2,Period2>& rhs) const    { return !(*this < rhs); }

    /// Get number of repetitions of period
    const Rep& count() const                                    { return _rep; }
    /// Get number of repetitions of period
    operator const Rep&() const                                 { return _rep; }

    /// Get common type between this duration and another
    template<class rhs>
    struct common
    {
        typedef Duration<   typename std::common_type<Rep,typename rhs::Rep>::type,
                            typename Period::template common<typename rhs::Period>::type
                        > type;
    };

private:
    Rep _rep;

public:
    /// Zero duration
    static const Duration zero;
    /// Minimum duration (negative reps)
    static const Duration min;
    /// Maximum duration (positive reps)
    static const Duration max;
};

/// operator+(Duration, Duration).  Returns a duration with a best-fit period and repetition type.
/** \relates Duration */
template<class Rep, class Period, class Rep2, class Period2>
typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type
operator+(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    typedef typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type DurCommon;
    return DurCommon(lhs) += rhs;
}

/// operator-(Duration, Duration).  Returns a duration with a best-fit period and repetition type.
/** \relates Duration */
template<class Rep, class Period, class Rep2, class Period2>
typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type
operator-(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    typedef typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type DurCommon;
    return DurCommon(lhs) -= rhs;
}

/// operator*(Duration, Duration).  Returns a duration with a best-fit period and repetition type.
/** \relates Duration */
template<class Rep, class Period, class Rep2, class Period2>
typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type
operator*(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    typedef typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type DurCommon;
    return DurCommon(lhs) *= rhs;
}

/// operator/(Duration, Duration).  Returns a duration with a best-fit period and repetition type.
/** \relates Duration */
template<class Rep, class Period, class Rep2, class Period2>
typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type
operator/(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    typedef typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type DurCommon;
    return DurCommon(lhs) /= rhs;
}

/// operator%(Duration, Duration).  Returns a duration with a best-fit period and repetition type.
/** \relates Duration */
template<class Rep, class Period, class Rep2, class Period2>
typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type
operator%(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    typedef typename Duration<Rep,Period>::template common<Duration<Rep2,Period2>>::type DurCommon;
    return DurCommon(lhs) %= rhs;
}

template<class Rep, class Period>
const Duration<Rep, Period> Duration<Rep, Period>::zero(0);

template<class Rep, class Period>
const Duration<Rep, Period> Duration<Rep, Period>::min(numeral<Rep>().min());

template<class Rep, class Period>
const Duration<Rep, Period> Duration<Rep, Period>::max(numeral<Rep>().max());

/// \name Duration types
/// @{
typedef Duration<int64, ratio::Nano>                            Nanosec;
typedef Duration<int64, ratio::Micro>                           Microsec;
typedef Duration<int64, ratio::Milli>                           Millisec;
typedef Duration<int64, ratio::Unit>                            Seconds;
typedef Duration<int,   Seconds::Period::mul<Ratio<60>>::type>  Minutes;
typedef Duration<int,   Minutes::Period::mul<Ratio<60>>::type>  Hours;
typedef Duration<int,   Hours::Period::mul<Ratio<24>>::type>    Days;
/// @}

}

