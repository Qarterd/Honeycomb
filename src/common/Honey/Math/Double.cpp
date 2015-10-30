// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Double.h"

namespace honey
{

const double Double_::zero          (0.);
const double Double_::smallest      (numeral<double>().smallest());
const double Double_::epsilon       (numeral<double>().epsilon());
const double Double_::zeroTol       (1e-08);
const double Double_::quarter       (0.25);
const double Double_::half          (0.5);
const double Double_::one           (1.);
const double Double_::sqrtTwo       (sqrt(2.));
const double Double_::two           (2.);
const double Double_::e             (2.7182818284590452353602874713527);
const double Double_::piEigth       (0.39269908169872415480783042290994);
const double Double_::piQuarter     (0.78539816339744830961566084581988);
const double Double_::piHalf        (1.5707963267948966192313216916398);
const double Double_::pi            (3.1415926535897932384626433832795);
const double Double_::piAndHalf     (4.7123889803846898576939650749193);
const double Double_::piTwo         (6.283185307179586476925286766559);
const double Double_::max           (numeral<double>().max());
const double Double_::inf           (numeral<double>().inf());
const double Double_::nan           (numeral<double>().nan());

}