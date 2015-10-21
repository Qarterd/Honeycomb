// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"

namespace honey
{

/// Class to hold compile-time finite rational numbers, ie. the fraction num / den.
template<int64 Num, int64 Den = 1>
struct Ratio
{
private:
    friend struct Ratio;
    template<class rhs> struct lessImpl;
public:

    static_assert(Den != 0, "Denominator can't be 0");

    static const int64 num = Num * mt::sign<Den>::value / mt::gcd<Num,Den>::value;
    static const int64 den = mt::abs<Den>::value / mt::gcd<Num,Den>::value;

    /// operator+
    template<class rhs>
    struct add
    {
    private:
        static const int64 gcd1 = mt::gcd<den, rhs::den>::value;
        static const int64 n = num * (rhs::den / gcd1) + rhs::num * (den / gcd1);
        static const int64 gcd2 = mt::gcd<n, gcd1>::value;

    public:
        typedef Ratio<n / gcd2, (den / gcd2) * (rhs::den / gcd1)> type;
    };

    /// operator-
    template<class rhs>
    struct sub                                          { typedef typename add<Ratio<-rhs::num, rhs::den>>::type type; };

    /// operator*
    template<class rhs>
    struct mul
    {
    private:
        static const int64 gcd1 = mt::gcd<num, rhs::den>::value;
        static const int64 gcd2 = mt::gcd<rhs::num, den>::value;
    public:
        typedef Ratio<(num / gcd1) * (rhs::num / gcd2), (den / gcd2) * (rhs::den / gcd1)> type;
    };

    /// operator/
    template<class rhs>
    struct div                                          { static_assert(rhs::num != 0, "Divide by 0"); typedef typename mul<Ratio<rhs::den, rhs::num>>::type type; };

    /// operator==
    template<class rhs>
    struct equal                                        : mt::Value<bool, num == rhs::num && den == rhs::den> {};

    /// operator!=
    template<class rhs>
    struct notEqual                                     : mt::Value<bool, !equal<rhs>::value> {};

    /// operator<
    template<class rhs>
    struct less                                         : mt::Value<bool, lessImpl<rhs>::value> {};

    /// operator<=
    template<class rhs>
    struct lessEqual                                    : mt::Value<bool, !rhs::template less<Ratio>::value> {};

    /// operator>
    template<class rhs>
    struct greater                                      : mt::Value<bool, rhs::template less<Ratio>::value> {};

    /// operator>=
    template<class rhs>
    struct greaterEqual                                 : mt::Value<bool, !less<rhs>::value> {};

private:
    template<class lhs, class rhs> struct lessCmpFrac;
    
    template<   class rhs,
                int64 q1 = num / den,
                int64 q2 = rhs::num / rhs::den,
                bool eq = q1 == q2>
    struct lessCmpWhole;

    /// Compare the signs of the ratios. Default case both ratios are positive, do whole test.
    template<   class rhs,
                bool = (num == 0 || rhs::num == 0 || (mt::sign<num>::value != mt::sign<rhs::num>::value)),
                bool = (mt::sign<num>::value == -1 && mt::sign<rhs::num>::value == -1)>
    struct lessCmpSign                                  : lessCmpWhole<rhs> {};

    /// One ratio is negative, trivial comparison
    template<class rhs>
    struct lessCmpSign<rhs, true, false>                : mt::Value<bool, (num < rhs::num)> {};

    /// Both ratios are negative, test positive wholes
    template<class rhs>
    struct lessCmpSign<rhs, false, true> :
        Ratio<-rhs::num, rhs::den>::
            template lessCmpWhole<
                Ratio<-num, den>
            > {};

    /// Private implementation
    template<class rhs> struct lessImpl                 : lessCmpSign<rhs> {};

    /// Compare the whole parts. Default case they are equal, compare the fractional parts
    template<class rhs, int64 q1, int64 q2, bool eq>
    struct lessCmpWhole :
        lessCmpFrac<
            Ratio<num % den, den>,
            Ratio<rhs::num % rhs::den, rhs::den>
        > {};

    /// Whole parts not equal, trivial comparison
    template<class rhs, int64 q1, int64 q2>
    struct lessCmpWhole<rhs, q1, q2, false>             : mt::Value<bool, (q1 < q2)> {};

    /// Test fractional parts.  Make fractional whole by inverting, then do recursive whole test Den2/Num2 < Den1/Num1
    template<class lhs, class rhs>
    struct lessCmpFrac :
        Ratio<rhs::den, rhs::num>::
            template lessCmpWhole<
                Ratio<lhs::den, lhs::num>
            > {};

    /// Fractional recursion end, Num1 != 0, Num2 == 0
    template<class lhs, int64 Den2>
    struct lessCmpFrac<lhs, Ratio<0, Den2>>             : mt::Value<bool, false> {};

    /// Fractional recursion end, Num1 == 0, Num2 != 0
    template<int64 Den1, class rhs>
    struct lessCmpFrac<Ratio<0, Den1>, rhs>             : mt::Value<bool, true> {};

    /// Fractional recursion end, Num1 == 0, Num2 == 0
    template<int64 Den1, int64 Den2>
    struct lessCmpFrac<Ratio<0, Den1>, Ratio<0, Den2>>  : mt::Value<bool, false> {};
};

/// Ratio types
namespace ratio
{
    typedef Ratio<1, 1000000000000000000>   Atto;
    typedef Ratio<1, 1000000000000000>      Femto;
    typedef Ratio<1, 1000000000000>         Pico;
    typedef Ratio<1, 1000000000>            Nano;
    typedef Ratio<1, 1000000>               Micro;
    typedef Ratio<1, 1000>                  Milli;
    typedef Ratio<1, 100>                   Centi;
    typedef Ratio<1, 10>                    Deci;
    typedef Ratio<1, 1>                     Unit;
    typedef Ratio<10, 1>                    Deca;
    typedef Ratio<100, 1>                   Hecto;
    typedef Ratio<1000, 1>                  Kilo;
    typedef Ratio<1000000, 1>               Mega;
    typedef Ratio<1000000000, 1>            Giga;
    typedef Ratio<1000000000000, 1>         Tera;
    typedef Ratio<1000000000000000, 1>      Peta;
    typedef Ratio<1000000000000000000, 1>   Exa;
}

}

/** \cond */
namespace std
{
/** \endcond */
    /// Get common ratio between two ratios
    /** \relates Ratio */
    template<honey::int64 Num, honey::int64 Den, honey::int64 Num2, honey::int64 Den2>
    struct common_type<honey::Ratio<Num,Den>, honey::Ratio<Num2,Den2>>
    {
    private:
        static const honey::int64 gcdNum = honey::mt::gcd<Num, Num2>::value;
        static const honey::int64 gcdDen = honey::mt::gcd<Den, Den2>::value;
    public:
        typedef honey::Ratio<gcdNum, (Den / gcdDen) * Den2> type;
    };
/** \cond */
}
/** \endcond */
