// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Ratio.h"
#include "Honey/Math/Numeral.h"

namespace honey { template<class Rep, class Period> class Duration; }

/** \cond */
namespace std
{
/** \endcond */
    /// Get common duration between two durations. The period of the resulting duration is the greatest common divisor.
    /** \relates Duration */
    template<class Rep, class Period, class Rep2, class Period2>
    struct common_type<honey::Duration<Rep,Period>, honey::Duration<Rep2,Period2>>
    {
        typedef honey::Duration<typename common_type<Rep,Rep2>::type, typename common_type<Period,Period2>::type> type;
    };
/** \cond */
}
/** \endcond */

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
    
    constexpr Duration() = default;
    /// Construct from repetition
    template<class Rep2>
    constexpr explicit Duration(const Rep2& rep)                : _rep(rep) {}
    /// Construct from another type of duration
    template<class Rep2, class Period2, class Ratio = typename Period2::template div<Period>::type>
    constexpr Duration(const Duration<Rep2,Period2>& d)         : _rep(d._rep * Ratio::num / Ratio::den) {}

    template<class Rep2, class Period2, class Ratio = typename Period2::template div<Period>::type>
    Duration& operator=(const Duration<Rep2,Period2>& rhs)      { _rep = rhs._rep * Ratio::num / Ratio::den; return *this; }

    constexpr Duration operator+() const                        { return *this; }
    Duration& operator++()                                      { ++_rep; return *this; }
    Duration operator++(int) const                              { return Duration(_rep++); }
    Duration& operator+=(const Duration& rhs)                   { _rep += rhs._rep; return *this; }

    constexpr Duration operator-() const                        { return Duration(-_rep); }
    Duration& operator--()                                      { --_rep; return *this; }
    Duration operator--(int) const                              { return Duration(_rep--); }
    Duration& operator-=(const Duration& rhs)                   { _rep -= rhs._rep; return *this; }

    Duration& operator*=(const Rep& rhs)                        { _rep *= rhs; return *this; }
    Duration& operator/=(const Rep& rhs)                        { _rep /= rhs; return *this; }
    Duration& operator%=(const Rep& rhs)                        { _rep %= rhs; return *this; }
    Duration& operator%=(const Duration& rhs)                   { _rep %= rhs._rep; return *this; }

    template<class Rep2, class Period2, class DurCommon = typename std::common_type<Duration, Duration<Rep2,Period2>>::type>
    constexpr bool operator==(const Duration<Rep2,Period2>& rhs) const  { return DurCommon(*this)._rep == DurCommon(rhs)._rep; }

    template<class Rep2, class Period2>
    constexpr bool operator!=(const Duration<Rep2,Period2>& rhs) const  { return !(*this == rhs); }

    template<class Rep2, class Period2, class DurCommon = typename std::common_type<Duration, Duration<Rep2,Period2>>::type>
    constexpr bool operator<(const Duration<Rep2,Period2>& rhs) const   { return DurCommon(*this)._rep < DurCommon(rhs)._rep; }

    template<class Rep2, class Period2>
    constexpr bool operator>(const Duration<Rep2,Period2>& rhs) const   { return rhs < *this; }

    template<class Rep2, class Period2>
    constexpr bool operator<=(const Duration<Rep2,Period2>& rhs) const  { return !(rhs < *this); }
    
    template<class Rep2, class Period2>
    constexpr bool operator>=(const Duration<Rep2,Period2>& rhs) const  { return !(*this < rhs); }

    /// Get number of repetitions of period
    constexpr Rep count() const                                 { return _rep; }

    /// Zero duration
    static constexpr Duration zero()                            { return Duration(0); }
    /// Minimum duration (negative reps)
    static constexpr Duration min()                             { return Duration(numeral<Rep>().min()); }
    /// Maximum duration (positive reps)
    static constexpr Duration max()                             { return Duration(numeral<Rep>().max()); }
    
private:
    Rep _rep;
};

/// \name Duration operators
/// \relates Duration
/// @{

/// operator+(Duration, Duration). Returns a duration with best-fit period and repetition types.
template<class Rep, class Period, class Rep2, class Period2,
    class DurCommon = typename std::common_type<Duration<Rep,Period>, Duration<Rep2,Period2>>::type>
constexpr DurCommon operator+(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    return DurCommon(DurCommon(lhs).count() + DurCommon(rhs).count());
}
/// operator-(Duration, Duration). Returns a duration with best-fit period and repetition types.
template<class Rep, class Period, class Rep2, class Period2,
    class DurCommon = typename std::common_type<Duration<Rep,Period>, Duration<Rep2,Period2>>::type>
constexpr DurCommon operator-(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    return DurCommon(DurCommon(lhs).count() - DurCommon(rhs).count());
}
/// operator*(Duration, Rep). Returns a duration of best-fit period and repetition types.
template<class Rep, class Period, class Rep2,
    class DurCommon = Duration<typename std::common_type<Rep,Rep2>::type, Period>>
constexpr DurCommon operator*(const Duration<Rep,Period>& lhs, const Rep2& rhs)
{
    return DurCommon(DurCommon(lhs).count() * rhs);
}
/// operator*(Rep, Duration). Returns a duration of best-fit period and repetition types.
template<class Rep, class Rep2, class Period,
    class DurCommon = Duration<typename std::common_type<Rep,Rep2>::type, Period>>
constexpr DurCommon operator*(const Rep& lhs, const Duration<Rep2,Period>& rhs)
{
    return rhs * lhs;
}
/// operator/(Duration, Rep). Returns a duration of best-fit period and repetition types.
template<class Rep, class Period, class Rep2,
    typename mt::disable_if<mt::isSpecializationOf<Rep2, Duration>::value, int>::type=0,
    class DurCommon = Duration<typename std::common_type<Rep,Rep2>::type, Period>>
constexpr DurCommon operator/(const Duration<Rep,Period>& lhs, const Rep2& rhs)
{
    return DurCommon(DurCommon(lhs).count() / rhs);
}
/// operator/(Duration, Duration). Returns a repetition of best-fit type.
template<class Rep, class Period, class Rep2, class Period2,
    class DurCommon = typename std::common_type<Duration<Rep,Period>, Duration<Rep2,Period2>>::type,
    class RepCommon = typename std::common_type<Rep,Rep2>::type>
constexpr RepCommon operator/(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    return DurCommon(lhs).count() / DurCommon(rhs).count();
}
/// operator%(Duration, Rep). Returns a duration of best-fit period and repetition types.
template<class Rep, class Period, class Rep2,
    typename mt::disable_if<mt::isSpecializationOf<Rep2, Duration>::value, int>::type=0,
    class DurCommon = Duration<typename std::common_type<Rep,Rep2>::type, Period>>
constexpr DurCommon operator%(const Duration<Rep,Period>& lhs, const Rep2& rhs)
{
    return DurCommon(DurCommon(lhs).count() % rhs);
}
/// operator%(Duration, Duration). Returns a duration of best-fit period and repetition types.
template<class Rep, class Period, class Rep2, class Period2,
    class DurCommon = typename std::common_type<Duration<Rep,Period>, Duration<Rep2,Period2>>::type>
constexpr DurCommon operator%(const Duration<Rep,Period>& lhs, const Duration<Rep2,Period2>& rhs)
{
    return DurCommon(DurCommon(lhs).count() % DurCommon(rhs).count());
}
/// @}

/// \name Duration types
/// \relates Duration
/// @{
typedef Duration<int64, ratio::Nano>                            Nanosec;
typedef Duration<int64, ratio::Micro>                           Microsec;
typedef Duration<int64, ratio::Milli>                           Millisec;
typedef Duration<int64, ratio::Unit>                            Seconds;
typedef Duration<int,   Seconds::Period::mul<Ratio<60>>::type>  Minutes;
typedef Duration<int,   Minutes::Period::mul<Ratio<60>>::type>  Hours;
typedef Duration<int,   Hours::Period::mul<Ratio<24>>::type>    Days;
/// @}

/// \name Duration literals
/// \relates Duration
/// @{

/// Construct Nanosec from integer literal (eg. 100_ns)
constexpr Nanosec operator"" _ns(unsigned long long int n)      { return Nanosec(static_cast<Nanosec::Rep>(n)); }
/// Construct Microsec from integer literal
constexpr Microsec operator"" _us(unsigned long long int n)     { return Microsec(static_cast<Microsec::Rep>(n)); }
/// Construct Millisec from integer literal
constexpr Millisec operator"" _ms(unsigned long long int n)     { return Millisec(static_cast<Millisec::Rep>(n)); }
/// Construct Seconds from integer literal
constexpr Seconds operator"" _s(unsigned long long int n)       { return Seconds(static_cast<Seconds::Rep>(n)); }
/// Construct Minutes from integer literal
constexpr Minutes operator"" _min(unsigned long long int n)     { return Minutes(static_cast<Minutes::Rep>(n)); }
/// Construct Hours from integer literal
constexpr Hours operator"" _h(unsigned long long int n)         { return Hours(static_cast<Hours::Rep>(n)); }
/// @}

}

/** \cond */
namespace std
{
/** \endcond */
    /// Duration to string
    /** \relates Duration */
    template<class Rep, class Period>
    ostream& operator<<(ostream& os, const honey::Duration<Rep,Period>& d)  { return os << d.count(); }
/** \cond */
}
/** \endcond */