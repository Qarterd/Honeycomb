// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Real.h"

namespace honey
{

/// Defines 64-bit floating point operations and constants
class Double_ : public RealBase<double>
{
public:
    /// Don't use quad as double type, double precision is good enough
    typedef Double_ DoubleType;

    static double abs(double x)                     { return fabs(x); }
    static double ceil(double x)                    { return ::ceil(x); }
    static double floor(double x)                   { return ::floor(x); }
    static double round(double x)                   { return ::floor(x + 0.5); }
    static double trunc(double x)                   { return (x < 0) ? -::floor(-x) : ::floor(x); }
    static double frac(double x)                    { double whole; return modf(x, &whole); }
    static double mod(double x, double y)           { return fmod(x, y); }
    static double sqrt(double x)                    { return ::sqrt(x); }
    static double exp(double x)                     { return ::exp(x); }
    static double pow(double x, double y)           { return ::pow(x, y); }
    static double log(double x)                     { return ::log(x); }
    static double sin(double x)                     { return ::sin(x); }
    static double asin(double x)                    { return ::asin(x); }
    static double cos(double x)                     { return ::cos(x); }
    static double acos(double x)                    { return ::acos(x); }
    static double tan(double x)                     { return ::tan(x); }
    static double atan(double x)                    { return ::atan(x); }
    static double atan2(double y, double x)         { return ::atan2(y, x); }

    static const double zero;
    static const double smallest;
    static const double epsilon;
    static const double zeroTol;
    static const double quarter;
    static const double half;
    static const double one;
    static const double sqrtTwo;
    static const double two;
    static const double e;
    static const double piEigth;
    static const double piQuarter;
    static const double piHalf;
    static const double pi;
    static const double piAndHalf;
    static const double piTwo;
    static const double max;
    static const double inf;
    static const double nan;
};

/// \name Real types
/// @{

/// `double` type
typedef Double_::Real Double;
/// @}

}

