// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey { namespace platform
{

class Condition
{
public:
    Condition();
    virtual ~Condition();

    void signal();
    void broadcast();

    void wait(UniqueLock<honey::Mutex>& lock)                                               { wait(lock, honey::MonoClock::TimePoint::max); }
    bool wait(UniqueLock<honey::Mutex>& lock, honey::MonoClock::Duration time)              { return wait(lock, honey::MonoClock::now() + time); }
    bool wait(UniqueLock<honey::Mutex>& external, honey::MonoClock::TimePoint time);

private:
    pthread_cond_t _handle;
};

} }
/** \endcond */
