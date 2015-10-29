// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

class SpinLock;

namespace platform
{

class Condition
{
public:
    Condition();
    virtual ~Condition();

    void signal();
    void broadcast();

    bool wait(UniqueLock<honey::Mutex>& external, honey::MonoClock::TimePoint time);

private:
    int _waitCount;
    UniquePtr<SpinLock> _waitLock;
    HANDLE _sema;
    HANDLE _timer;
    HANDLE _waitDone;
    bool _broadcast;
};

} }
/** \endcond */
