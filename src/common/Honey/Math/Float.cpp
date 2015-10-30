// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Float.h"

namespace honey
{

const float Float_::zero        (0.f);
const float Float_::smallest    (numeral<float>().smallest());
const float Float_::epsilon     (numeral<float>().epsilon());
const float Float_::zeroTol     (1e-06f);
const float Float_::quarter     (0.25f);
const float Float_::half        (0.5f);
const float Float_::one         (1.f);
const float Float_::sqrtTwo     (sqrt(2.f));
const float Float_::two         (2.f);
const float Float_::e           (2.718281828f);
const float Float_::piEigth     (0.392699082f);
const float Float_::piQuarter   (0.785398163f);
const float Float_::piHalf      (1.570796327f);
const float Float_::pi          (3.141592654f);
const float Float_::piAndHalf   (4.712388980f);
const float Float_::piTwo       (6.283185307f);
const float Float_::max         (numeral<float>().max());
const float Float_::inf         (numeral<float>().inf());
const float Float_::nan         (numeral<float>().nan());

}