// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Future/Future.h"

namespace honey
{

/** \cond */
namespace future { namespace priv
{
    template<class R> struct wrappedResult<SharedFuture<R>>     { typedef R type; };
    
    template<class R> struct unwrapOnReady<SharedFuture<R>> : mt::FuncptrBase, SmallAllocatorObject
    {
        unwrapOnReady(Promise<R>&& promise) : promise(move(promise)) {}
        void operator()(StateBase& src)
        {
            if (src.ready) src.ex ? promise.setException(*src.ex) : promise.setValue(static_cast<State<R>&>(src).result());
            delete_(this);
        }
        Promise<R> promise;
    };
    
    template<> struct unwrapOnReady<SharedFuture<void>> : mt::FuncptrBase, SmallAllocatorObject
    {
        unwrapOnReady(Promise<void>&& promise) : promise(move(promise)) {}
        void operator()(StateBase& src)
        {
            if (src.ready) src.ex ? promise.setException(*src.ex) : promise.setValue();
            delete_(this);
        }
        Promise<void> promise;
    };
} }
/** \endcond */

/// Shared future, allows multiple access to a future function result.
template<class R>
class SharedFuture : public FutureBase, public FutureCommon<SharedFuture<R>, R>
{
    template<class, class> friend class FutureCommon;
    template<class> friend class Future;
    template<class> friend class SharedFuture;
    
public:
    typedef typename std::conditional<mt::isRef<R>::value || std::is_void<R>::value, R, typename mt::addConstRef<R>::type>::type Result;
    typedef future::priv::State<R> State;
    
    SharedFuture()                                              : _state(nullptr) {}
    SharedFuture(Future<R>&& rhs)                               : _state(move(rhs._state)) {}
    SharedFuture(const SharedFuture& rhs)                       : _state(nullptr) { operator=(rhs); }
    SharedFuture(SharedFuture&& rhs)                            : _state(nullptr) { operator=(move(rhs)); }

    SharedFuture& operator=(const SharedFuture& rhs)            { _state = rhs._state; return *this; }
    SharedFuture& operator=(SharedFuture&& rhs)                 { _state = move(rhs._state); return *this; }

    /// Get the future result, waiting if necessary. Throws any exception stored in the result. The result can be retrieved repeatedly.
    Result get() const;
    
    /// Get the shared state
    State& __state() const                                      { assert(_state); return *_state; }
    
protected:
    virtual StateBase* stateBase() const                        { return _state; }
    
private:
    SharedPtr<State> _state;
};

template<class R>
auto SharedFuture<R>::get() const -> Result
{
    wait();
    if (_state->ex) _state->ex->raise();
    return _state->result();
}

template<>
inline void SharedFuture<void>::get() const
{
    wait();
    if (_state->ex) _state->ex->raise();
}

/// Create a shared future that is immediately ready with the value
template<class T, class R = typename std::decay<T>::type>
SharedFuture<R> SharedFutureCreate(T&& val)
{
    Promise<R> promise;
    promise.setValue(forward<T>(val));
    return promise.future().share();
}

inline SharedFuture<void> SharedFutureCreate()
{
    Promise<void> promise;
    promise.setValue();
    return promise.future().share();
}

}
