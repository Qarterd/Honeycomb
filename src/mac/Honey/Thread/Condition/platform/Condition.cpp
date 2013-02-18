// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Condition/Condition.h"
#include "Honey/Thread/Thread.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey { namespace platform
{

Condition::Condition()          { verify(!pthread_cond_init(&_handle, nullptr)); }
Condition::~Condition()         { pthread_cond_destroy(&_handle); }

void Condition::signal()        { pthread_cond_signal(&_handle); }
void Condition::broadcast()     { pthread_cond_broadcast(&_handle); }

bool Condition::wait(UniqueLock<honey::Mutex>& external, honey::MonoClock::TimePoint time)
{
    honey::thread::priv::InterruptWait _(reinterpret_cast<honey::Condition&>(*this), external.mutex());

    timespec time_;
    //platform bug, should allow numeral<decltype(time_.tv_sec)>().max()
    time_.tv_sec = Alge::min(Seconds(time.time()).count(), numeral<int>().max());
    time_.tv_nsec = time.time() % Seconds(1);
    return !pthread_cond_timedwait(&_handle, &external.mutex().handle(), &time_);
}

} }
/** \endcond */



