// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Real.h"

namespace honey
{

/// Defines 128-bit floating point operations and constants
class Quad_ : public RealBase<float128>
{
public:
    typedef Quad_ DoubleType;

    static float128 abs(float128 x)                     { return fabsl(x); }
    static float128 ceil(float128 x)                    { return ceill(x); }
    static float128 floor(float128 x)                   { return floorl(x); }
    static float128 round(float128 x)                   { return floorl(x + 0.5); }
    static float128 trunc(float128 x)                   { return (x < 0) ? -floorl(-x) : floorl(x); }
    static float128 frac(float128 x)                    { float128 whole; return modfl(x, &whole); }
    static float128 mod(float128 x, float128 y)         { return fmodl(x, y); }
    static float128 sqrt(float128 x)                    { return sqrtl(x); }
    static float128 exp(float128 x)                     { return expl(x); }
    static float128 pow(float128 x, float128 y)         { return powl(x, y); }
    static float128 log(float128 x)                     { return logl(x); }
    static float128 sin(float128 x)                     { return sinl(x); }
    static float128 asin(float128 x)                    { return asinl(x); }
    static float128 cos(float128 x)                     { return cosl(x); }
    static float128 acos(float128 x)                    { return acosl(x); }
    static float128 tan(float128 x)                     { return tanl(x); }
    static float128 atan(float128 x)                    { return atanl(x); }
    static float128 atan2(float128 y, float128 x)       { return atan2l(y, x); }

    static const float128 zero;
    static const float128 smallest;
    static const float128 epsilon;
    static const float128 zeroTol;
    static const float128 quarter;
    static const float128 half;
    static const float128 one;
    static const float128 sqrtTwo;
    static const float128 two;
    static const float128 e;
    static const float128 piEigth;
    static const float128 piQuarter;
    static const float128 piHalf;
    static const float128 pi;
    static const float128 piAndHalf;
    static const float128 piTwo;
    static const float128 max;
    static const float128 inf;
    static const float128 nan;
};

/// \name Real types
/// @{

/// `float128` type
typedef Quad_::Real Quad;
/// @}

}

