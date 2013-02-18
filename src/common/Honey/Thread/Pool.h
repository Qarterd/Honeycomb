// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Condition/Lock.h"

namespace honey { namespace thread
{

/// Thread pool, spreads task execution across a pool of re-usable threads. Uses a work-stealing queue to ensure threads are never waiting.
class Pool : public SharedObj<Pool>
{
public:
    /// All tasks must inherit from this class.  std::function is not used here to avoid the operator() virtual call.
    struct Task : mt::FuncptrBase
    {
        virtual void log(const String& file, int line, const String& msg) const = 0;
        virtual bool logEnabled() const = 0;
    };
    
    /**
      * \param workerCount      Number of workers
      * \param workerTaskMax    Max size of per-worker task queue, overflow will be pushed onto pool queue
      */
    Pool(int workerCount, int workerTaskMax);
    ~Pool();

    /// Schedule a task for execution
    template<class Func>
    void enqueue(Func&& f)                          { enqueue_(TaskPtr(forward<Func>(f))); }
    
    /// Get the current task object of the calling thread.  Must be called from inside a task, returns null otherwise.
    static Task* current()                          { auto& ptr = Worker::current()._task; return ptr ? &(*ptr) : nullptr; }
    
private:
    struct TaskPtr : mt::Funcptr<void ()>
    {
        TaskPtr() = default;
        TaskPtr(nullptr_t)                          : TaskPtr() {}
        template<class F> TaskPtr(F&& f)            : mt::Funcptr<void ()>(forward<F>(f)) {}
    
        Task& operator*() const                     { assert(base); return static_cast<Task&>(*base); }
        Task* operator->() const                    { assert(base); return static_cast<Task*>(base); }
    };
    
    class Worker
    {
        friend class Pool;
    public:
        Worker(Pool& pool);
        
        /// Get the current worker of the calling thread
        static Worker& current()                    { assert(*_current); return **_current; }
        
    private:
        void start();
        void join();
        void run();
        
        /// Get next task
        TaskPtr next();
        
        Pool& _pool;
        Thread _thread;
        bool _active;
        ConditionLock _cond;
        bool _condWait;
        deque<TaskPtr> _tasks;
        TaskPtr _task;
        static thread::Local<Worker*> _current;
    };
    
    void enqueue_(TaskPtr task);
    
    int                 _workerTaskMax;
    Mutex               _lock;
    vector<UniquePtr<Worker>> _workers;
    deque<TaskPtr>      _tasks;
};

} }
