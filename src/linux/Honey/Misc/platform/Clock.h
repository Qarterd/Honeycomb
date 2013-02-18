// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

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
};

} }
/** \endcond */
