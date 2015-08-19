// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Debug.h"

namespace honey
{
/** \cond */
namespace numeral_priv { template<class T> struct Info; }
/** \endcond */

class Float_;
class Double_;
class Quad_;

/// Numeric type information, use numeral() to get instance safely from a static context
template<class T> class Numeral;

/// Numeric type info for integer types
template<class T>
class NumeralInt
{
    typedef numeral_priv::Info<T> Info;
    Info _info;
    
public:
    /// Integer representation of type
    typedef typename Info::Int              Int;
    /// Real representation of type
    typedef typename Info::Real             Real;
    /// Real operations and constants class
    typedef typename Info::Real_            Real_;

    /// Size of type in bits
    static const int sizeBits               = sizeof(T)*8;

    /// Minimum possible value for type (negative for signed types)
    constexpr T min() const                 { return _info.min; }
    /// Maximum possible value for type
    constexpr T max() const                 { return _info.max; }
};

/// Numeric type info for floating point types
template<class T>
class NumeralFloat
{
    typedef numeral_priv::Info<T> Info;
    Info _info;
    
public:
    /// Integer representation of type
    typedef typename Info::Int              Int;
    /// Real representation of type
    typedef typename Info::Real             Real;
    /// Real operations and constants class
    typedef typename Info::Real_            Real_;

    /// Size of type in bits
    static const int sizeBits               = sizeof(T)*8;

    /// Minimum possible value for type
    constexpr T min() const                 { return _info.min(); }
    /// Maximum possible value for type
    constexpr T max() const                 { return _info.max(); }
    /// Smallest representable value (close to zero)
    constexpr T smallest() const            { return _info.smallest(); }
    /// Smallest value such that 1.0 + epsilon != 1.0
    constexpr T epsilon() const             { return _info.epsilon(); }
    /// Infinity. ie. 1.0 / 0.0
    constexpr T inf() const                 { return _info.inf(); }
    /// Not a number. ie. 0.0 / 0.0, sqrt(-1)
    constexpr T nan() const                 { return _info.nan(); }
};


/// Get numeric type info safely from a static context
template<class T>
constexpr Numeral<T> numeral()              { return Numeral<T>(); }
/// Get numeric type info of deduced type
template<class T>
constexpr Numeral<T> numeral(const T&)      { return numeral<T>(); }

/// \name Safe conversion between integer types
/// Asserts that value is within result type integer range.
/// @{

/// convert signed int to unsigned with size equal or larger
template<class To, class From, typename std::enable_if<std::is_signed<From>::value && std::is_unsigned<To>::value && sizeof(To) >= sizeof(From), int>::type=0>
To numeric_cast(From s)                     { assert(s >= 0); return static_cast<To>(s); }
/// convert unsigned int to signed with size equal or larger
template<class To, class From, typename std::enable_if<std::is_unsigned<From>::value && std::is_signed<To>::value && sizeof(To) >= sizeof(From), int>::type=0>
To numeric_cast(From u)                     { assert(static_cast<To>(u) >= 0); return static_cast<To>(u); }
/// convert same sign int to size equal or larger, does nothing
template<class To, class From, typename std::enable_if<std::is_signed<From>::value == std::is_signed<To>::value && sizeof(To) >= sizeof(From), int>::type=0>
To numeric_cast(From i)                     { return static_cast<To>(i); }
/// convert signed int to size smaller
template<class To, class From, typename std::enable_if<std::is_signed<From>::value && sizeof(To) < sizeof(From), int>::type=0>
To numeric_cast(From s)                     { assert(s >= static_cast<From>(numeral<To>().min()) && s <= static_cast<From>(numeral<To>().max())); return static_cast<To>(s); }
/// convert unsigned int to size smaller
template<class To, class From, typename std::enable_if<std::is_unsigned<From>::value && sizeof(To) < sizeof(From), int>::type=0>
To numeric_cast(From u)                     { assert(u <= static_cast<From>(numeral<To>().max())); return static_cast<To>(u); }
/// @}

/** \cond */
namespace numeral_priv
{
    template<> struct Info<int8>
    {
        typedef int8    Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const int8 min       = -0x7F - 1; 
        static const int8 max       = 0x7F;
    };
    
    template<> struct Info<uint8> 
    {
        typedef uint8   Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const uint8 min      = 0;
        static const uint8 max      = 0xFFU;
    };

    template<> struct Info<int16>
    {
        typedef int16   Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const int16 min      = -0x7FFF - 1;
        static const int16 max      = 0x7FFF;
    };

    template<> struct Info<uint16>
    {
        typedef uint16  Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const uint16 min     = 0;
        static const uint16 max     = 0xFFFFU;
    };
    
    template<> struct Info<int32>
    {
        typedef int32   Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const int32 min      = -0x7FFFFFFF - 1;
        static const int32 max      = 0x7FFFFFFF;
    };

    template<> struct Info<uint32>
    {
        typedef uint32  Int;
        typedef float   Real;
        typedef Float_  Real_;
        static const uint32 min     = 0;
        static const uint32 max     = 0xFFFFFFFFU;
    };

    template<> struct Info<int64>
    {
        typedef int64   Int;
        typedef double  Real;
        typedef Double_ Real_;
        static const int64 min      = -0x7FFFFFFFFFFFFFFFL - 1;
        static const int64 max      = 0x7FFFFFFFFFFFFFFFL;
    };

    template<> struct Info<uint64>
    {
        typedef uint64  Int;
        typedef double  Real;
        typedef Double_ Real_;
        static const uint64 min     = 0;
        static const uint64 max     = 0xFFFFFFFFFFFFFFFFUL;
    };

    template<> struct Info<float>
    {
        typedef int32   Int;
        typedef float   Real;
        typedef Float_  Real_;

        constexpr float min() const         { return -3.402823466e+38f; }
        constexpr float max() const         { return 3.402823466e+38f; }
        constexpr float smallest() const    { return 1.175494351e-38f; }
        constexpr float epsilon() const     { return 1.192092896e-07f; }
        constexpr float one() const         { return 1.f; }
        constexpr float inf() const         { return 1.f / (1.f - one()); }
        constexpr float nan() const         { return 0.f / (1.f - one()); }
    };

    template<> struct Info<double>
    {
        typedef int64   Int;
        typedef double  Real;
        typedef Double_ Real_;

        constexpr double min() const        { return -1.7976931348623158e+308; }
        constexpr double max() const        { return 1.7976931348623158e+308; }
        constexpr double smallest() const   { return 2.2250738585072014e-308; }
        constexpr double epsilon() const    { return 2.2204460492503131e-016; }
        constexpr double one() const        { return 1.0; }
        constexpr double inf() const        { return 1.0 / (1.0 - one()); }
        constexpr double nan() const        { return 0.0 / (1.0 - one()); }
    };
}

template<> class Numeral<int8> : public NumeralInt<int8> {};
template<> class Numeral<uint8> : public NumeralInt<uint8> {};
template<> class Numeral<int16> : public NumeralInt<int16> {};
template<> class Numeral<uint16> : public NumeralInt<uint16> {};
template<> class Numeral<int32> : public NumeralInt<int32> {};
template<> class Numeral<uint32> : public NumeralInt<uint32> {};
template<> class Numeral<int64> : public NumeralInt<int64> {};
template<> class Numeral<uint64> : public NumeralInt<uint64> {};
template<> class Numeral<float> : public NumeralFloat<float> {};
template<> class Numeral<double> : public NumeralFloat<double> {};
/** \endcond */

}

#include "Honey/Math/platform/Numeral.h"

