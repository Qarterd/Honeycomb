// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Future/Promise.h"
#include "Honey/Memory/SmallAllocator.h"

namespace honey
{

namespace future
{
    enum class Status
    {
        ready,      ///< the future result is ready
        timeout     ///< timeout expired
    };
}

/// Base class for Future types
class FutureBase : mt::NoCopy
{
public:
    typedef future::priv::StateBase StateBase;
    
    /// Check if this instance has state and can be used.  State can be transferred out to another instance through move-assignment.
    bool valid() const                                          { return stateBase(); }
    /// Check if result is ready. \throws future::NoState
    bool ready() const                                          { auto state = stateBase(); if (!state) throw_ future::NoState(); return state->ready; }
    
    /// Wait until result is ready. \throws future::NoState
    void wait() const                                           { wait(MonoClock::TimePoint::max); }
    /// Wait until result is ready or until an amount of time has passed. \throws future::NoState
    template<class Rep, class Period>
    future::Status wait(Duration<Rep,Period> time) const        { return wait(MonoClock::now() + time); }
    /// Wait until result is ready or until a certain time. \throws future::NoState
    template<class Clock, class Dur>
    future::Status wait(TimePoint<Clock,Dur> time) const
    {
        auto state = stateBase();
        if (!state) throw_ future::NoState();
        ConditionLock::Scoped _(state->waiters);
        while (!state->ready) if (!state->waiters.wait(time)) return future::Status::timeout;
        return future::Status::ready;
    }

    /// Get the shared state
    StateBase& __stateBase() const                              { assert(stateBase()); return *stateBase(); }
    
protected:
    virtual StateBase* stateBase() const = 0;
};

template<class R> class Future;
template<class R> class SharedFuture;

/** \cond */
namespace future { namespace priv
{
    template<class R> struct wrappedResult                      { typedef mt::tag<0> type; }; //dummy
    template<class R> struct wrappedResult<Future<R>>           { typedef R type; };
    
    template<class R> struct unwrapOnReady;
    
    template<class R> struct unwrapOnReady<Future<R>> : mt::FuncptrBase, SmallAllocatorObject
    {
        unwrapOnReady(Promise<R>&& promise) : promise(move(promise)) {}
        void operator()(StateBase& src)
        {
            if (src.ready) src.ex ? promise.setException(*src.ex) : promise.setValue(move(static_cast<State<R>&>(src).result()));
            delete_(this);
        }
        Promise<R> promise;
    };
    
    template<> struct unwrapOnReady<Future<void>> : mt::FuncptrBase, SmallAllocatorObject
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

/// Mixin for common future methods
template<class Subclass, class R>
class FutureCommon
{
public:
    /// Append a continuation function that will be called when this future is ready.  The ready future is passed in (no wait on future.get()).
    template<class Sched, class Func>
    Future<typename std::result_of<Func(Subclass)>::type>
        then(Sched&& sched, Func&& f);
    template<class Func>
    Future<typename std::result_of<Func(Subclass)>::type>
        then(Func&& f);
    
    /// For wrapped futures Future<Future<R>>, returns a proxy Future<R> that will be ready when the inner future is ready.
    /**
      * If the wrapped future is shared (ie. Future<SharedFuture<R>>), the result will be copied, otherwise the result will be moved.
      * If the outer or inner future throws an exception, the proxy future will throw it on get().
      */
    template<   class R2 = typename future::priv::wrappedResult<R>::type,
                typename std::enable_if<mt::True<R2>::value && std::is_base_of<FutureBase, R>::value, int>::type=0>
    Future<R2> unwrap()
    {
        using namespace future::priv;
        if (!subc()._state) throw_ future::NoState();
        Promise<R2> promise{SmallAllocator<int>()};
        auto future = promise.future();
        
        struct onReady : mt::FuncptrBase, SmallAllocatorObject
        {
            onReady(Promise<R2>&& promise) : promise(move(promise)) {}
            void operator()(StateBase& src)
            {
                if (src.ready)
                {
                    if (src.ex)
                        this->promise.setException(*src.ex);
                    else
                    {
                        auto& wrapped = static_cast<State<R>&>(src).result();
                        wrapped._state->addOnReady(*new future::priv::unwrapOnReady<R>(move(this->promise)));
                    }
                }
                delete_(this);
            }
            Promise<R2> promise;
        };
    
        subc()._state->addOnReady(*new onReady(move(promise)));
        return future;
    }
    
private:
    /// Get the subclass that inherited from this base class
    const Subclass& subc() const                                { return static_cast<const Subclass&>(*this); }
    Subclass& subc()                                            { return static_cast<Subclass&>(*this); }
};

/// Unique future, guarantees sole access to a future function result.
template<class R>
class Future : public FutureBase, public FutureCommon<Future<R>, R>
{
    template<class> friend class Promise;
    template<class, class> friend class FutureCommon;
    template<class> friend class Future;
    template<class> friend class SharedFuture;
    
public:
    typedef R Result;
    typedef future::priv::State<R> State;
    
    Future()                                                    : _state(nullptr) {}
    Future(Future&& rhs) noexcept                               : _state(nullptr) { operator=(move(rhs)); }
    
    Future& operator=(Future&& rhs)                             { _state = move(rhs._state); return *this; }

    /// Share the future between multiple future objects. This future is rendered invalid.
    SharedFuture<R> share()                                     { return SharedFuture<R>(move(*this)); }

    /// Get the future result, waiting if necessary. Throws any exception stored in the result.
    /**
      * \throws future::NoState     if the result has been retrieved with get() more than once
      */
    R get();
    
    /// Get the shared state
    State& __state() const                                      { assert(_state); return *_state; }
    
protected:
    virtual StateBase* stateBase() const                        { return _state; }

private:
    explicit Future(const SharedPtr<State>& state)              : _state(state) {}

    template<class R_>
    struct getResult        { static R_&& func(const SharedPtr<State>& state) { return move(state->result()); } };
    template<class R_>
    struct getResult<R_&>   { static R_& func(const SharedPtr<State>& state) { return state->result(); } };
    
    SharedPtr<State> _state;
};

template<class R>
R Future<R>::get()
{
    wait();
    R res = getResult<R>::func(_state);
    auto ex = _state->ex;
    _state = nullptr;
    if (ex) ex->raise();
    return res;
}

template<>
inline void Future<void>::get()
{
    wait();
    auto ex = _state->ex;
    _state = nullptr;
    if (ex) ex->raise();
}

/// Create a future that is immediately ready with the value
template<class T, class R = typename std::decay<T>::type>
Future<R> FutureCreate(T&& val)
{
    Promise<R> promise;
    promise.setValue(forward<T>(val));
    return promise.future();
}

inline Future<void> FutureCreate()
{
    Promise<void> promise;
    promise.setValue();
    return promise.future();
}

}
