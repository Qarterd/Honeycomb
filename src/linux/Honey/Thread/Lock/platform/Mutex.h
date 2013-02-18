// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey { namespace platform
{

/// Provides timed mutex functionality to a windows critical section
class Mutex
{
public:
    typedef pthread_mutex_t Handle;

    Mutex();
    virtual ~Mutex();

    void lock();
    void unlock();
    bool tryLock();
    
    Handle& handle()                { return _handle; }

private:
    Handle _handle;
};

} }
/** \endcond */
