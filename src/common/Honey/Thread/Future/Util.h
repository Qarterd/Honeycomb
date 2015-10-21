// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Future/SharedFuture.h"
#include "Honey/Thread/Future/PackagedTask.h"
#include "Honey/Thread/Pool.h"
#include "Honey/Misc/Range.h"

namespace honey { namespace future
{

//====================================================
// waitAll / waitAny
//====================================================

inline void waitAll() {} //dummy to catch empty parameter pack
/// Wait until all futures are ready
template<class Future, class... Futures, typename mt::disable_if<mt::isRange<Future>::value, int>::type=0>
void waitAll(Future&& f, Futures&&... fs)                   { f.wait(); waitAll(forward<Futures>(fs)...); }

/// Wait until all futures in a range are ready or until a certain time
template<class Range, class Clock, class Dur, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
void waitAll(Range&& range, TimePoint<Clock,Dur> time)      { for (auto& e : range) e.wait(time); }
/// Wait until all futures in a range are ready or until an amount of time has passed
template<class Range, class Rep, class Period, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
void waitAll(Range&& range, Duration<Rep,Period> time)      { return waitAll(forward<Range>(range), time == time.max() ? MonoClock::TimePoint::max() : MonoClock::now() + time); }
/// Wait until all futures in a range are ready
template<class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
void waitAll(Range&& range)                                 { return waitAll(forward<Range>(range), MonoClock::TimePoint::max()); }

/** \cond */
namespace priv
{
    /// Helper to wait on multiple futures concurrently
    class waitAny : public mt::FuncptrBase
    {
    public:
        waitAny();
        ~waitAny();

        void add(const FutureBase& f);
        int wait(MonoClock::TimePoint time);
        void operator()(StateBase& src);
        
    private:
        /// waitAny() needs state that is expensive to create,
        /// so instead of creating the state every call, each thread has its own state cache.
        struct ThreadData
        {
            vector<StateBase*> states;
            ConditionLock cond;
        };
        static mt_global((thread::Local<ThreadData>), threadData,);
        
        ThreadData& td;
        StateBase* readyState;
    };
}
/** \endcond */

/// Wait until any futures are ready, returns index of ready future
template<class Future, class... Futures, typename mt::disable_if<mt::isRange<Future>::value, int>::type=0>
int waitAny(Future&& f, Futures&&... fs)
{
    priv::waitAny waiter;
    array<const FutureBase*, sizeof...(Futures)+1> futures = {&f, &fs...};
    for (auto& e : futures) waiter.add(*e);
    return waiter.wait(MonoClock::TimePoint::max());
}

/// Wait until any futures in a range are ready or until a certain time, returns iterator to ready future or range end if timed out
template<class Range, class Clock, class Dur, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
auto waitAny(Range&& range, TimePoint<Clock,Dur> time) -> mt_iterOf(range)
{
    priv::waitAny waiter;
    for (auto& e : range) waiter.add(e);
    int index = waiter.wait(time);
    return index >= 0 ? next(begin(range), index) : end(range);
}
/// Wait until any futures in a range are ready or until an amount of time has passed, returns iterator to ready future or range end if timed out
template<class Range, class Rep, class Period, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
auto waitAny(Range&& range, Duration<Rep,Period> time) -> mt_iterOf(range)      { return waitAny(forward<Range>(range), time == time.max() ? MonoClock::TimePoint::max() : MonoClock::now() + time); }
/// Wait until any futures in a range are ready, returns iterator to ready future
template<class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0>
auto waitAny(Range&& range) -> mt_iterOf(range)                                 { return waitAny(forward<Range>(range), MonoClock::TimePoint::max()); }

//====================================================
// async
//====================================================

/** \cond */
namespace priv
{
    template<class Func>
    struct Task : SmallAllocatorObject, thread::Pool::Task
    {
        Task(Func&& f)                      : f(forward<Func>(f)) {}
        virtual ~Task() {}
 
        void operator()()                   { f(); delete_(this); }
        
        Func f;
        
    protected:
        virtual bool traceEnabled() const;
    };
    
    //TODO: this is a work-around for clang's broken std::function, it can't handle move-only bind args
    template<class R, class Func, class... Args>
    struct Bind
    {
        Bind(Func&& f, Args&&... args)      : f(forward<Func>(f)), args(forward<Args>(args)...) {}
        //clang's std::function tries to copy, just move
        Bind(const Bind& rhs)               : Bind(move(const_cast<Bind&>(rhs))) {}
        Bind(Bind&&) = default;
        
        R operator()()                      { return func(mt::make_idxseq<sizeof...(Args)>()); }
        template<szt... Seq>
        R func(mt::idxseq<Seq...>)          { return f(forward<Args>(get<Seq>(args))...); }
        
        Func f;
        tuple<Args...> args;
    };
    
    template<class Func, class... Args>
    struct Bind<void, Func, Args...>
    {
        Bind(Func&& f, Args&&... args)      : f(forward<Func>(f)), args(forward<Args>(args)...) {}
        Bind(const Bind& rhs)               : Bind(move(const_cast<Bind&>(rhs))) {}
        Bind(Bind&&) = default;
        
        void operator()()                   { func(mt::make_idxseq<sizeof...(Args)>()); }
        template<szt... Seq>
        void func(mt::idxseq<Seq...>)       { f(forward<Args>(get<Seq>(args))...); }
        
        Func f;
        tuple<Args...> args;
    };
    
    template<class Func, class... Args, class R = typename std::result_of<Func(Args...)>::type>
    Bind<R, Func, Args...> bind(Func&& f, Args&&... args)   { return Bind<R, Func, Args...>(forward<Func>(f), forward<Args>(args)...); }
}
/** \endcond */

struct AsyncSched;
struct AsyncSched_tag {};
AsyncSched& async_createSingleton();

struct AsyncSched : thread::Pool, AsyncSched_tag
{
    AsyncSched(int workerCount, int workerTaskMax)          : thread::Pool(workerCount, workerTaskMax) {}
    
    static AsyncSched& inst()                               { static SharedPtr<AsyncSched> inst = &async_createSingleton(); return *inst; }
    
    template<class Func>
    void operator()(Func&& f)                               { enqueue(*new priv::Task<Func>(forward<Func>(f))); }
    
    /// Whether to log task execution flow
    static bool trace;
};

#ifndef future_async_createSingleton
    /// Default implementation
    inline AsyncSched& async_createSingleton()              { return *new AsyncSched(3, 5); }
#endif

/** \cond */
template<class Func>
bool priv::Task<Func>::traceEnabled() const                 { return AsyncSched::trace; }
/** \endcond */

/// Call a function asynchronously, returns a future with the result of the function call.
template<class Sched, class Func, class... Args, typename std::enable_if<mt::is_base_of<AsyncSched_tag, Sched>::value, int>::type=0>
Future<typename std::result_of<Func(Args...)>::type>
    async(Sched&& sched, Func&& f, Args&&... args)
{
    typedef typename std::result_of<Func(Args...)>::type R;
    PackagedTask<R()> task(priv::bind(forward<Func>(f), forward<Args>(args)...), SmallAllocator<int>());
    auto future = task.future();
    sched(move(task));
    return future;
}

/// Async using global scheduler
/**
  * To provide a custom global scheduler define `future_async_createSingleton` and implement future::async_createSingleton().
  */
template<class Func, class... Args>
Future<typename std::result_of<Func(Args...)>::type>
    async(Func&& f, Args&&... args)                         { return async(AsyncSched::inst(), forward<Func>(f), forward<Args>(args)...); }
    
}

//====================================================
// then
//====================================================

template<class Subclass, class R>
template<class Sched, class Func>
auto FutureCommon<Subclass, R>::then(Sched&& sched, Func&& f) -> Future<typename std::result_of<Func(Subclass)>::type>
{
    using namespace future::priv;
    if (!subc()._state) throw_ future::NoState();
    typedef typename std::result_of<Func(Subclass)>::type R2;
    Promise<R2> promise{SmallAllocator<int>()};
    auto future = promise.future();
    
    struct onReady : mt::FuncptrBase, SmallAllocatorObject
    {
        onReady(Subclass&& cont, Promise<R2>&& promise, Sched&& sched, Func&& f) :
            cont(move(cont)), promise(move(promise)), sched(forward<Sched>(sched)), f(forward<Func>(f)) {}
        
        void operator()(StateBase& src)
        {
            if (src.ready)
            {
                PackagedTask<R2()> task(future::priv::bind(forward<Func>(this->f), move(this->cont)),
                                        SmallAllocator<int>(), move(this->promise));
                this->sched(move(task));
            }
            delete_(this);
        }
        
        Subclass cont;
        Promise<R2> promise;
        Sched sched;
        Func f;
    };

    subc()._state->addOnReady(*new onReady(move(subc()), move(promise), forward<Sched>(sched), forward<Func>(f)));
    return future;
}

template<class Subclass, class R>
template<class Func>
auto FutureCommon<Subclass, R>::then(Func&& f) -> Future<typename std::result_of<Func(Subclass)>::type>
                                                            { return then(future::AsyncSched::inst(), forward<Func>(f)); }

namespace future
{

//====================================================
// whenAll / whenAny
//====================================================

/** \cond */
namespace priv
{
    template<class Func, class Futures, szt... Seq>
    void when_init(Func& func, Futures& fs, mt::idxseq<Seq...>)
                                                            { mt_unpackEval(get<Seq>(fs).__state().addOnReady(func)); }
    
    template<class Result>
    struct whenAll_onReady
    {        
        template<class Futures, szt... Seq>
        static void func(Promise<Result>& promise, Futures& fs, mt::idxseq<Seq...>)
                                                            { promise.setValue(make_tuple(get<Seq>(fs).__state().result()...)); }
        template<class Range>
        static void func(Promise<Result>& promise, Range& range)
                                                            { Result res; for (auto& e : range) res.push_back(e.__state().result()); promise.setValue(res); }
    };
    
    template<>
    struct whenAll_onReady<void>
    {        
        template<class Futures, szt... Seq>
        static void func(Promise<void>& promise, Futures&, mt::idxseq<Seq...>)
                                                            { promise.setValue(); }
        template<class Range>
        static void func(Promise<void>& promise, Range&)
                                                            { promise.setValue(); }
    };
    
    template<class Futures, szt... Seq>
    int whenAny_valIndex(StateBase& src, Futures& fs, mt::idxseq<Seq...>)
                                                            { return mt::valIndex(&src, &get<Seq>(fs).__state()...); }
    template<class Range>
    int whenAny_valIndex(StateBase& src, Range& range)      { int i = -1; return find(range, [&](auto& e) { return ++i, &src == &e.__state(); }) != end(range) ? i : -1; }
    
    template<class Result_>
    struct whenAny_onReady
    {
        template<class Futures, szt... Seq, class Result = tuple<int, Result_>>
        static void func(Promise<Result>& promise, Futures& fs, mt::idxseq<Seq...>, int i)
                                                            { promise.setValue(make_tuple(i, mt::valAt(i, get<Seq>(fs)...).__state().result())); }
        template<class Range, class Result = tuple<typename mt::iterOf<Range>::type, Result_>>
        static void func(Promise<Result>& promise, Range& range, int i)
                                                            { auto it = next(begin(range), i); promise.setValue(make_tuple(it, it->__state().result())); }
    };
    
    template<>
    struct whenAny_onReady<void>
    {        
        template<class Futures, szt... Seq>
        static void func(Promise<int>& promise, Futures&, mt::idxseq<Seq...>, int i)
                                                            { promise.setValue(i); }
        template<class Range, class Result = typename mt::iterOf<Range>::type>
        static void func(Promise<Result>& promise, Range& range, int i)
                                                            { promise.setValue(next(begin(range), i)); }
    };
}
/** \endcond */

inline Future<tuple<>> whenAll()            { return FutureCreate(tuple<>()); }

/// Returns a future to either a tuple of the results of all futures, or the first exception thrown by the futures.
template<   class... Futures,
            class Result_ = typename std::decay<typename mt::removeRef<mt::typeAt<0, Futures...>>::type::Result>::type,
            class Result = typename std::conditional<   std::is_same<Result_, void>::value,
                                                        void,
                                                        tuple<typename std::decay<typename mt::removeRef<Futures>::type::Result>::type...>>::type>
Future<Result> whenAll(Futures&&... fs)
{
    using namespace future::priv;
    Promise<Result> promise{SmallAllocator<int>()};
    auto future = promise.future();
    
    struct onReady : mt::FuncptrBase, SmallAllocatorObject
    {
        onReady(Promise<Result>&& promise, Futures&&... fs) :
            promise(move(promise)), fs(forward<Futures>(fs)...), count(0), ready(0), max(sizeof...(fs)) {}
        
        void operator()(StateBase& src)
        {
            SpinLock::Scoped _(this->lock);
            if (src.ready && !this->promise.__state().ready)
            {
                if (src.ex)
                    this->promise.setException(src.ex);
                else if (++this->ready == this->max)
                    priv::whenAll_onReady<Result>::func(this->promise, this->fs, mt::make_idxseq<sizeof...(Futures)>());
            }
            if (++this->count == this->max) { _.unlock(); delete_(this); }
        }
        
        Promise<Result> promise;
        tuple<Futures...> fs;
        szt count;
        szt ready;
        szt max;
        SpinLock lock;
    };

    auto& func = *new onReady(move(promise), forward<Futures>(fs)...);
    priv::when_init(func, func.fs, mt::make_idxseq<sizeof...(Futures)>());
    return future;
}

/// whenAll() for a range of futures
template<   class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0,
            class Result_ = typename std::decay<typename mt::elemOf<Range>::type::Result>::type,
            class Result = typename std::conditional<   std::is_same<Result_, void>::value,
                                                        void,
                                                        vector<Result_>>::type>
Future<Result> whenAll(Range&& range)
{
    using namespace future::priv;
    Promise<Result> promise{SmallAllocator<int>()};
    auto future = promise.future();
    
    struct onReady : mt::FuncptrBase, SmallAllocatorObject
    {
        onReady(Promise<Result>&& promise, Range&& range) :
            promise(move(promise)), range(forward<Range>(range)), count(0), ready(0), max(countOf(range)) {}
        
        void operator()(StateBase& src)
        {
            SpinLock::Scoped _(this->lock);
            if (src.ready && !this->promise.__state().ready)
            {
                if (src.ex)
                    this->promise.setException(src.ex);
                else if (++this->ready == this->max)
                    priv::whenAll_onReady<Result>::func(this->promise, this->range);
            }
            if (++this->count == this->max) { _.unlock(); delete_(this); }
        }
        
        Promise<Result> promise;
        Range range;
        szt count;
        szt ready;
        szt max;
        SpinLock lock;
    };

    auto& func = *new onReady(move(promise), forward<Range>(range));
    for (auto& e : func.range) e.__state().addOnReady(func);
    return future;
}

/// Returns a future to a tuple of the index of the ready future and its result
template<   class... Futures,
            class Result_ = typename std::decay<typename mt::removeRef<mt::typeAt<0, Futures...>>::type::Result>::type,
            class Result = typename std::conditional<   std::is_same<Result_, void>::value,
                                                        int,
                                                        tuple<int, Result_>>::type>
Future<Result> whenAny(Futures&&... fs)
{
    using namespace future::priv;
    Promise<Result> promise{SmallAllocator<int>()};
    auto future = promise.future();
    
    struct onReady : mt::FuncptrBase, SmallAllocatorObject
    {
        onReady(Promise<Result>&& promise, Futures&&... fs) :
            promise(move(promise)), fs(forward<Futures>(fs)...), count(0), max(sizeof...(fs)) {}
        
        void operator()(StateBase& src)
        {
            SpinLock::Scoped _(this->lock);
            if (src.ready && !this->promise.__state().ready)
            {
                if (src.ex)
                    this->promise.setException(src.ex);
                else
                {
                    auto seq = mt::make_idxseq<sizeof...(Futures)>();
                    priv::whenAny_onReady<Result_>::func(this->promise, this->fs, seq, whenAny_valIndex(src, this->fs, seq));
                }
            }
            if (++this->count == this->max) { _.unlock(); delete_(this); }
        }
        
        Promise<Result> promise;
        tuple<Futures...> fs;
        szt count;
        szt max;
        SpinLock lock;
    };

    auto& func = *new onReady(move(promise), forward<Futures>(fs)...);
    priv::when_init(func, func.fs, mt::make_idxseq<sizeof...(Futures)>());
    return future;
}

/// whenAny() for a range of futures
template<   class Range, typename std::enable_if<mt::isRange<Range>::value, int>::type=0,
            class Result_ = typename std::decay<typename mt::elemOf<Range>::type::Result>::type,
            class Result = typename std::conditional<   std::is_same<Result_, void>::value,
                                                        typename mt::iterOf<Range>::type,
                                                        tuple<typename mt::iterOf<Range>::type, Result_>>::type>
Future<Result> whenAny(Range&& range)
{
    using namespace future::priv;
    Promise<Result> promise{SmallAllocator<int>()};
    auto future = promise.future();
    
    struct onReady : mt::FuncptrBase, SmallAllocatorObject
    {
        onReady(Promise<Result>&& promise, Range&& range) :
            promise(move(promise)), range(forward<Range>(range)), count(0), max(countOf(range)) {}
        
        void operator()(StateBase& src)
        {
            SpinLock::Scoped _(this->lock);
            if (src.ready && !this->promise.__state().ready)
            {
                if (src.ex)
                    this->promise.setException(src.ex);
                else
                    priv::whenAny_onReady<Result_>::func(this->promise, this->range, whenAny_valIndex(src, this->range));
            }
            if (++this->count == this->max) { _.unlock(); delete_(this); }
        }
        
        Promise<Result> promise;
        Range range;
        szt count;
        szt max;
        SpinLock lock;
    };

    auto& func = *new onReady(move(promise), forward<Range>(range));
    for (auto& e : func.range) e.__state().addOnReady(func);
    return future;
}

} }
