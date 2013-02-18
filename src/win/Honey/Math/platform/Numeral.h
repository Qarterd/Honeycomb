// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

namespace numeral_priv
{
    template<> struct Info<float128>
    {
        typedef float128    Signed;
        typedef float128    Unsigned;
        typedef int128      Int;
        typedef float128    Real;
        typedef Quad_       RealT;
        static const bool isSigned  = true;
        static const bool isInteger = false;

        // Disable divide by zero warning
        #pragma warning(push)
        #pragma warning(disable:4723)

        Info() :
            min(        -1.7976931348623158e+308),
            max(        1.7976931348623158e+308),
            smallest(   2.2250738585072014e-308),
            epsilon(    2.2204460492503131e-016),
            one(        1.0),
            inf(        1.0 / (1.0 - one)),
            nan(        0.0 / (1.0 - one)) {}

        #pragma warning(pop)

        const float128 min;
        const float128 max;
        const float128 smallest;
        const float128 epsilon;
        const float128 one;
        const float128 inf;
        const float128 nan;
    };
}

template<> class Numeral<float128> : public NumeralFloat<float128> {};

}
/** \endcond */