// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Real.h"

namespace honey
{

class Double_;

/// Defines floating point operations and constants
class Float_ : public RealBase<float>
{
public:
    typedef Double_ DoubleType;

    static float abs(float x)                       { return fabsf(x); }
    static float ceil(float x)                      { return ceilf(x); }
    static float floor(float x)                     { return floorf(x); }
    static float round(float x)                     { return floorf(x + 0.5f); }
    static float trunc(float x)                     { return (x < 0.f) ? -floorf(-x) : floorf(x); }
    static float frac(float x)                      { float whole; return modff(x, &whole); }
    static float mod(float x, float y)              { return fmodf(x, y); }
    static float sqrt(float x)                      { return sqrtf(x); }
    static float exp(float x)                       { return expf(x); }
    static float pow(float x, float y)              { return powf(x, y); }
    static float log(float x)                       { return logf(x); }
    static float sin(float x)                       { return sinf(x); }
    static float asin(float x)                      { return asinf(x); }
    static float cos(float x)                       { return cosf(x); }
    static float acos(float x)                      { return acosf(x); }
    static float tan(float x)                       { return tanf(x); }
    static float atan(float x)                      { return atanf(x); }
    static float atan2(float y, float x)            { return atan2f(y, x); }

    static const float zero;
    static const float smallest;
    static const float epsilon;
    static const float zeroTol;
    static const float quarter;
    static const float half;
    static const float one;
    static const float sqrtTwo;
    static const float two;
    static const float e;
    static const float piEigth;
    static const float piQuarter;
    static const float piHalf;
    static const float pi;
    static const float piAndHalf;
    static const float piTwo;
    static const float max;
    static const float inf;
    static const float nan;
};

/// \name Real types
/// @{

/// `float` type
typedef Float_::Real Float;
/// @}

}

