// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Condition/Lock.h"
#include "Honey/Thread/LockFree/Queue.h"

namespace honey { namespace thread
{

/// Spreads task execution across a pool of re-usable threads. Uses a lock-free work-stealing queue to ensure workers are never idle.
class Pool : public SharedObj<Pool>
{
public:
    /// All tasks must inherit from this class
    struct Task
    {
        friend class Pool;
        friend class Worker;
        
        virtual void operator()() = 0;

    protected:
        virtual void trace(const String& file, int line, const String& msg) const;
        virtual bool traceEnabled() const           { return false; }
    };
    
    /**
      * \param workerCount      Number of workers
      * \param workerTaskMax    Max size of per-worker task queue, overflow will be pushed onto pool queue
      */
    Pool(szt workerCount, szt workerTaskMax);
    ~Pool();

    /// Schedule a task for execution
    void enqueue(Task& task);
    
    /// Get the current task object of the calling thread.  Must be called from inside a task, returns null otherwise.
    static Task* current()                          { auto& ptr = Worker::current()._task; return ptr ? &(*ptr) : nullptr; }
    
private:
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
        Task* next();
        
        Pool&                       _pool;
        Thread                      _thread;
        bool                        _active;
        ConditionLock               _cond;
        bool                        _condWait;
        SpinLock                    _condOne;
        lockfree::Queue<Task*>      _tasks;
        Task*                       _task;
        static thread::Local<Worker*> _current;
    };
    
    const szt                   _workerTaskMax;
    vector<UniquePtr<Worker>>   _workers;
    lockfree::Queue<Task*>      _tasks;
};

} }
