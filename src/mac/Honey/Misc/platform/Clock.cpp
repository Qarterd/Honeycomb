// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Misc/Clock.h"
#include <chrono>

/** \cond */
namespace honey { namespace platform
{

template<class Subclass>
typename SystemClock<Subclass>::TimePoint SystemClock<Subclass>::now()
{
    return TimePoint(std::chrono::system_clock::now().time_since_epoch().count());
}

template struct SystemClock<honey::SystemClock>;



template<class Subclass>
typename MonoClock<Subclass>::TimePoint MonoClock<Subclass>::now()
{
    return TimePoint(std::chrono::steady_clock::now().time_since_epoch().count());
}

template struct MonoClock<honey::MonoClock>;


} }
/** \endcond */



