// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Lock/Spin.h"

namespace honey
{

/// Wraps a value so that it is calculated only when needed.  A lock synchronizes access.
template<class T, class Eval = function<void(T&)>, class Pred = function<bool()>>
class lazy
{
public:
    /**
      * \param eval     Function that will be called to evaluate the lazy value.
      *                 A reference to the wrapped value is provided as the first arg. `eval` is called after the lock is acquired.
      * \param pred     Optional predicate function to check if dirty.
      *                 The lazy value is dirty if isDirty() or `pred` return true.
      *                 `pred` is called once before (for early return) and once after the lock is acquired.
      *                 Because `pred` is called on every access, it should use atomics and avoid locks.
      */
    lazy(const Eval& eval = nullptr, const Pred& pred = nullptr)
                                                    : _dirty(true), _pred(pred), _eval(eval) {}

    void setDirty(bool dirty)                       { _dirty = dirty; }
    bool isDirty() const                            { return _dirty; }

    void setPred(const Pred& pred)                  { _pred = pred; }
    void setEval(const Eval& eval)                  { _eval = eval; }

    /// Direct access to the wrapped value (ie. does not evaluate)
    const T& raw() const                            { return _val; }
    T& raw()                                        { return _val; }

    /// Evaluate the lazy value
    const T& operator*() const                      { return get(); }
    T& operator*()                                  { return get(); }
    const T* operator->() const                     { return &get(); }
    T* operator->()                                 { return &get(); }
    /// Evaluate the lazy value
    operator const T&() const                       { return get(); }
    operator T&()                                   { return get(); }

    /// Evaluate the lazy value. Only evaluates if dirty. The args are passed to the predicate and eval function.
    template<class... Args>
    T& get(Args&&... args)
    {
        SpinLock::Scoped _(_lock);
        if (!(isDirty() || (_pred && _pred(args...)))) return _val;
        _eval(_val, args...);
        _dirty = false;
        return _val;
    }
    
    template<class... Args>
    const T& get(Args&&... args) const              { return const_cast<lazy*>(this)->get(forward<Args>(args)...); }

private:
    T               _val;
    Atomic<bool>    _dirty;
    Pred            _pred;
    Eval            _eval;
    SpinLock        _lock;
};

/// Create a lazy value from a function that returns a value. Ex. `auto lazy = lazyCreate([] { return T(); });`
/** \relates lazy */
template<class Eval>
auto lazyCreate(Eval&& eval) -> lazy<decltype(eval())>
{
    typedef decltype(eval()) R;
    return lazy<R>([=](R& val) mutable { val = eval(); });
}

}
