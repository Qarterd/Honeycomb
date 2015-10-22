// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Future/Util.h"
#include "Honey/Graph/Dep.h"

namespace honey
{

class PeriodicTask;
class PeriodicSched;
/** \cond */
//for shared ptr member var, is_base_of needs a defined class
namespace mt { template<> struct is_base_of<honey::priv::SharedObj_tag, PeriodicTask> : std::true_type {}; }
/** \endcond */

/// Base class of `PeriodicTask_`, returned by scheduler. \see PeriodicSched
class PeriodicTask : public SharedObj<PeriodicTask>, thread::Pool::Task
{
    friend class PeriodicSched;
    friend struct mt::Funcptr<void ()>;
    
public:
    typedef SharedPtr<PeriodicTask> Ptr;
    typedef function<void ()> Func;

    /// Future result when cancelled
    struct Cancelled : Exception                    { EXCEPTION(Cancelled) };
    
    virtual ~PeriodicTask() {}
    
    /// Get the current task object. Must be called from a task functor.
    static PeriodicTask& current();
    
    /// Check if task is scheduled or executing
    bool active() const                             { return _state != State::idle; }
    /// Returns time remaining until task is due for execution (task is due at zero time or less)
    MonoClock::Duration delay() const               { return _due.load() - MonoClock::now(); }
    
    /// Unschedule task from further execution. If the task is awaiting execution then its future will throw exception Cancelled.
    void cancel();
    /// Request an interrupt in the executing task's thread. \see Thread::interrupt()
    void interrupt(const Exception::ConstPtr& e = new thread::Interrupted)   { Mutex::Scoped _(_lock); if (_thread) _thread->interrupt(e); }
    /// Check whether an interrupt has been requested for the executing task's thread
    bool interruptRequested()                       { Mutex::Scoped _(_lock); return _thread ? _thread->interruptRequested() : false; }
    
    /// Set task's thread execution scheduling priority. \see Thread::setPriority()
    void setPriority(int priority)                  { Mutex::Scoped _(_lock); _priority = priority; if (_thread) _thread->setPriority(_priority); }
    /// Get task's thread execution scheduling priority. \see Thread::getPriority()
    int getPriority() const                         { return _priority; }
    
    /// Get id used for debug output
    const Id& id() const                            { return _id; }
    /// Get task info for prepending to a log record
    String info() const;
    
protected:
    enum class State
    {
        idle,           ///< Not active
        wait,           ///< Waiting for next period
        exec            ///< Executing functor
    };
    
    PeriodicTask(PeriodicSched& sched, optional<MonoClock::Duration> period, optional<MonoClock::Duration> delay, const Id& id);

    virtual void exec() = 0;
    virtual void readyFunctor(bool reset) = 0;
    virtual void cancelFunctor() = 0;
    
    void operator()();
    
    virtual void trace(const String& file, int line, const String& msg) const;
    virtual bool traceEnabled() const;

    PeriodicSched&                      _sched;
    optional<MonoClock::Duration>       _period;
    optional<MonoClock::Duration>       _delay;
    Id                                  _id;
    Mutex                               _lock;
    Ptr                                 _self;
    Atomic<State>                       _state;
    Atomic<MonoClock::TimePoint>        _due;
    bool                                _cancelled;
    Thread*                             _thread;
    int                                 _priority;
};

/// Holds a functor and period information, returned by scheduler. \see PeriodicSched
template<class Result>
class PeriodicTask_ : public PeriodicTask
{
    friend class PeriodicSched;

public:
    typedef SharedPtr<PeriodicTask_> Ptr;

    /// Get future from which delayed result can be retrieved
    /**
      * \throws future::FutureAlreadyRetrieved      if future() has been called more than once per task execution
      */
    Future<Result> future()                         { return _func.future(); }

    /// Wrapper for PeriodicTask::current()
    static PeriodicTask_& current()                 { return static_cast<PeriodicTask_&>(PeriodicTask::current()); }

private:
    template<class Func>
    PeriodicTask_(PeriodicSched& sched, Func&& f, optional<MonoClock::Duration> period, optional<MonoClock::Duration> delay, const Id& id)
                                                    : PeriodicTask(sched, period, delay, id), _func(forward<Func>(f)) {}
    
    virtual void exec()                             { _func.invoke_delayedReady(); }
    virtual void readyFunctor(bool reset)           { _func.setReady(reset); }
    virtual void cancelFunctor()                    { _func.setFunc([]{ throw_ Cancelled(); }); _func(); }
    
    PackagedTask<Result ()> _func;
};

/// Scheduler that executes tasks periodically or after a delay given a pool of threads
class PeriodicSched
{
    friend class PeriodicTask;
    
public:
    /// Get singleton, uses global future::AsyncSched pool
    static mt_global(PeriodicSched, inst, (future::AsyncSched::inst()));
    
    /**
      * \param pool     Shared ref to thread pool with which all tasks will be enqueued.
      */
    PeriodicSched(thread::Pool& pool);

    ~PeriodicSched();
    
    /// Schedule a task for execution
    /**
      * \param f        the function to execute
      * \param period   Execute function every `period` amount of time.
      *                 If not specified then the function will execute only once.
      *                 If execution takes longer than the period, then any subsequent execution will start immediately (not concurrently).
      * \param delay    Delay first execution for an amount of time.
      *                 If not specified then the function will be delayed for the amount of time specified by `period`.
      * \param id       display id for debugging
      * \return         the scheduled task
      */
    template<class Func, class PeriodicTask_ = PeriodicTask_<typename std::result_of<Func()>::type>>
    typename PeriodicTask_::Ptr schedule(   Func&& f, optional<MonoClock::Duration> period = optnull,
                                            optional<MonoClock::Duration> delay = optnull, const Id& id = idnull)
    {
        typename PeriodicTask_::Ptr task(new PeriodicTask_(*this, forward<Func>(f), period, delay, id));
        add(*task);
        return task;
    }
    
    /// Whether to log task execution flow
    static bool trace;
    
private:
    enum class Action
    {
        add,
        remove
    };
    
    void add(PeriodicTask& task);
    void remove(PeriodicTask& task);
    
    void run();
    
    SharedPtr<thread::Pool>                             _pool;
    Thread                                              _thread;
    bool                                                _active;
    ConditionLock                                       _cond;
    bool                                                _condWait;
    multimap<MonoClock::TimePoint, PeriodicTask::Ptr>   _tasks;
    vector<PeriodicTask::Ptr>                           _due;
    vector<tuple<Action, PeriodicTask::Ptr>>            _actions;
};

inline bool PeriodicTask::traceEnabled() const      { return PeriodicSched::trace; }

}
