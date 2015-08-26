// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
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
    bool wait(UniqueLock<honey::Mutex>& lock, honey::MonoClock::TimePoint time);

private:
    pthread_cond_t _handle;
};

} }
/** \endcond */
