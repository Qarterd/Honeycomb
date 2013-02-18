// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Double.h"

/** \cond */
namespace honey { namespace platform
{

template<class Subclass>
struct SystemClock
{
    typedef TimePoint<Subclass, Microsec> TimePoint;

    static TimePoint now();
};

template<class Subclass>
struct MonoClock
{
    typedef TimePoint<Subclass, Nanosec> TimePoint;

    static TimePoint now();

private:
    static Double _toNano;
    static LARGE_INTEGER _start;
    static bool _init;
    static bool init();
};

template<class Subclass> Double MonoClock<Subclass>::_toNano;
template<class Subclass> LARGE_INTEGER MonoClock<Subclass>::_start;
template<class Subclass> bool MonoClock<Subclass>::_init = init();

} }
/** \endcond */
