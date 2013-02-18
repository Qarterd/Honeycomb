// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"

namespace honey
{

/// Run a function at scope exit. See ScopeGuard() to create.
template<class F>
class ScopeGuard_ : mt::NoCopy
{
public:
    ScopeGuard_(F&& func)                   : _func(forward<F>(func)), _engaged(true) {}
    ScopeGuard_(ScopeGuard_&& rhs) noexcept : _func(move(rhs._func)), _engaged(move(rhs._engaged)) { rhs.release(); }
    ~ScopeGuard_()                          { if (_engaged) _func(); }
    
    /// Disengage the guard so the function isn't run at scope exit
    void release()                          { _engaged = false; }
private:
    F _func;
    bool _engaged;
};

/// Create a scope guard of deduced type. Call with lambda: `auto guard = ScopeGuard([] {...});`
/** \relates ScopeGuard_ */
template<class F>
ScopeGuard_<F> ScopeGuard(F&& func)         { return ScopeGuard_<F>(forward<F>(func)); }

}
