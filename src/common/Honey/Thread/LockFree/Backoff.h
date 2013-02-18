// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"

namespace honey { namespace lockfree
{

/// Exponential backoff algorithm.  Backoff spins for first X ticks, then sleeps with a time that doubles every X ticks thereafter.
class Backoff
{
public:

    /**
      * \param spin         Whether spinning is enabled
      * \param timeMin      Min sleep time
      * \param timeMax      Max sleep time
      * \param tickThresh   Tick threshold used to determine whether to spin or sleep, and when to grow the sleep time.
      */
    Backoff(bool spin = true, Nanosec timeMin = 100, Nanosec timeMax = Microsec(100), int tickThresh = 5) :
        _spin(spin),
        _timeMin(timeMin),
        _timeMax(timeMax),
        _tickThresh(tickThresh)
    {}

    /// Increase tick count by `ticks`.  Increases the amount of time that backoff will wait.
    void inc(int ticks = 1)
    {
        Local& l = _l;
        for (l.tick += ticks; l.tick >= _tickThresh; l.tick -= _tickThresh)
        {
            //Deactivate spin
            if (l.spin) { l.spin = false; continue; }
            //Grow sleep time
            l.time = l.time*2 + 1;
            if (l.time > _timeMax) l.time = _timeMax;
        }
    }

    /// Decrease tick count by `ticks`.  Decreases the amount of time that backoff will wait.
    void dec(int ticks = 1)
    {
        Local& l = _l;
        for (l.tick -= ticks; l.tick <= -_tickThresh; l.tick += _tickThresh)
        {
            //Activate spin
            if (l.time == _timeMin) { l.spin = _spin; continue; }
            //Shrink sleep time
            l.time = l.time/2;
            if (l.time < _timeMin) l.time = _timeMin;
        }
    }

    /// Perform backoff, suspend thread
    void wait()                             { Local& l = _l; if (l.spin) thread::current::pause(); else thread::current::sleep(l.time); }

    /// Reset backoff to initial state
    void reset()                            { Local& l = _l; l.spin = _spin; l.time = _timeMin; l.tick = 0; }

private:

    struct Local
    {
        bool                spin;
        Nanosec             time;
        int                 tick;
    };

    bool                    _spin;
    Nanosec                 _timeMin;
    Nanosec                 _timeMax;
    int                     _tickThresh;
    thread::Local<Local>    _l;
};

} }
