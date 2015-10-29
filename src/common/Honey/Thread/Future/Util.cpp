// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Thread/Future/Util.h"

namespace honey { namespace future
{

/** \cond */
namespace priv
{
    waitAny::waitAny()  : td(*threadData()), readyState(nullptr) {}

    waitAny::~waitAny()
    {
        for (auto& e : td.states)
        {
            ConditionLock::Scoped _(e->waiters);
            stdutil::eraseVal(e->onReady, this);
        }
        td.states.clear();
    }

    void waitAny::add(const FutureBase& f)
    {
        auto& state = f.__stateBase();
        state.addOnReady(*this);
        td.states.push_back(&state);
    }
        
    int waitAny::wait(MonoClock::TimePoint time)
    {
        ConditionLock::Scoped _(td.cond);
        do
        {
            auto it = find(td.states, [&](auto& e) { return e == readyState; });
            if (it != td.states.end()) return (int)(it - td.states.begin());
        } while (!td.states.empty() && td.cond.wait(time));
        return -1;
    }

    void waitAny::operator()(StateBase& src)
    {
        ConditionLock::Scoped _(td.cond);
        if (readyState) return;
        readyState = &src;
        td.cond.signal();
    }
}
/** \endcond */

bool AsyncSched::trace = false;

} }
