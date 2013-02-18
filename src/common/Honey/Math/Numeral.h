// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"

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
protected:
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
    T min() const                           { return _info.min; }
    /// Maximum possible value for type
    T max() const                           { return _info.max; }
};

/// Numeric type info for floating point types
template<class T>
class NumeralFloat : public NumeralInt<T>
{
public:
    /// Smallest representable value (close to zero)
    T smallest() const                      { return this->_info.smallest; }
    /// Smallest value such that 1.0 + epsilon != 1.0
    T epsilon() const                       { return this->_info.epsilon; }
    /// Infinity. ie. 1.0 / 0.0
    T inf() const                           { return this->_info.inf; }
    /// Not a number. ie. 0.0 / 0.0, sqrt(-1)
    T nan() const                           { return this->_info.nan; }
};


/// Get numeric type info safely from a static context
template<class T>
const Numeral<T>& numeral()                 { static Numeral<T> _obj; return _obj; }

/// Get numeric type info of deduced type
template<class T>
const Numeral<T>& numeral(const T&)         { return numeral<T>(); }

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
        
        Info() :
            min(        -3.402823466e+38f),
            max(        3.402823466e+38f),
            smallest(   1.175494351e-38f),
            epsilon(    1.192092896e-07f),
            one(        1.f),
            inf(        1.f / (1.f - one)),
            nan(        0.f / (1.f - one)) {}

        const float min;
        const float max;
        const float smallest;
        const float epsilon;
        const float one;
        const float inf;
        const float nan;
    };

    template<> struct Info<double>
    {
        typedef int64   Int;
        typedef double  Real;
        typedef Double_ Real_;

        Info() :
            min(        -1.7976931348623158e+308),
            max(        1.7976931348623158e+308),
            smallest(   2.2250738585072014e-308),
            epsilon(    2.2204460492503131e-016),
            one(        1.0),
            inf(        1.0 / (1.0 - one)),
            nan(        0.0 / (1.0 - one)) {}

        const double min;
        const double max;
        const double smallest;
        const double epsilon;
        const double one;
        const double inf;
        const double nan;
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

