// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

namespace numeral_priv
{
    template<> struct Info<float128>
    {
        typedef int128      Int;
        typedef float128    Real;
        typedef Quad_       Real_;

        constexpr float128 min() const { return -1.7976931348623158e+308; }
        constexpr float128 max() const { return 1.7976931348623158e+308; }
        constexpr float128 smallest() const { return 2.2250738585072014e-308; }
        constexpr float128 epsilon() const { return 2.2204460492503131e-016; }
        constexpr float128 one() const { return 1.0; }
        #pragma warning(push)
        #pragma warning(disable:4723)
        constexpr float128 inf() const { return 1.0 / (1.0 - one()); }
        constexpr float128 nan() const { return 0.0 / (1.0 - one()); }
        #pragma warning(pop)
    };
}

template<> class Numeral<float128> : public NumeralFloat<float128> {};

}
/** \endcond */