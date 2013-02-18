// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Misc/Clock.h"
#include "Honey/String/Stream.h"

/** \cond */
namespace honey { namespace platform
{

template<class Subclass>
typename SystemClock<Subclass>::TimePoint SystemClock<Subclass>::now()
{
    FILETIME f;
    GetSystemTimeAsFileTime(&f);
    ULARGE_INTEGER t;
    t.LowPart = f.dwLowDateTime;
    t.HighPart = f.dwHighDateTime;

    //Convert from windows 100 nanosecond period
    static const uint64 toMicro = 10;
    //Convert from windows epoch (Jan 1, 1601) to Unix epoch (Jan 1, 1970)
    static const uint64 toUnixEpoch = 11644473600000000ULL;

    return TimePoint(t.QuadPart/toMicro - toUnixEpoch);
}

template struct SystemClock<honey::SystemClock>;



template<class Subclass>
typename MonoClock<Subclass>::TimePoint MonoClock<Subclass>::now()
{
    LARGE_INTEGER now;
    verify(QueryPerformanceCounter(&now));
    return TimePoint((now.QuadPart - _start.QuadPart) * _toNano);
}

template<class Subclass>
bool MonoClock<Subclass>::init()
{
    LARGE_INTEGER freq;
    verify(QueryPerformanceFrequency(&freq));
    _toNano = (Double)Nanosec::Period::den / freq.QuadPart;
    verify(QueryPerformanceCounter(&_start));
    return true;
}

template struct MonoClock<honey::MonoClock>;


} }
/** \endcond */



