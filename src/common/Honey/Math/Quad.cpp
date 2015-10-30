// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Quad.h"

namespace honey
{

const float128 Quad_::zero          (0.);
const float128 Quad_::smallest      (numeral<float128>().smallest());
const float128 Quad_::epsilon       (numeral<float128>().epsilon());
const float128 Quad_::zeroTol       (1e-12);
const float128 Quad_::quarter       (0.25);
const float128 Quad_::half          (0.5);
const float128 Quad_::one           (1.);
const float128 Quad_::sqrtTwo       (sqrt(2.));
const float128 Quad_::two           (2.);
const float128 Quad_::e             (2.7182818284590452353602874713527L);
const float128 Quad_::piEigth       (0.39269908169872415480783042290994L);
const float128 Quad_::piQuarter     (0.78539816339744830961566084581988L);
const float128 Quad_::piHalf        (1.5707963267948966192313216916398L);
const float128 Quad_::pi            (3.1415926535897932384626433832795L);
const float128 Quad_::piAndHalf     (4.7123889803846898576939650749193L);
const float128 Quad_::piTwo         (6.283185307179586476925286766559L);
const float128 Quad_::max           (numeral<float128>().max());
const float128 Quad_::inf           (numeral<float128>().inf());
const float128 Quad_::nan           (numeral<float128>().nan());

}