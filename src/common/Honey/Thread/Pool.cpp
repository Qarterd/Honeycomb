// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Pool.h"
#include "Honey/Thread/Future/Util.h"
#include "Honey/Misc/Log.h"

namespace honey { namespace thread
{

#ifndef FINAL
    #define Pool_trace(taskptr, msg)    if ((taskptr)->traceEnabled()) (taskptr)->trace(__FILE__, __LINE__, (msg));
#else
    #define Pool_trace(...) {}
#endif

void Pool::Task::trace(const String& file, int line, const String& msg) const
{
    Log::inst() << log::level::debug <<
        "[" << log::srcFilename(file) << ":" << line << "] " <<
        "[Task: " << std::hex << reinterpret_cast<intptr_t>(this) << std::dec << ":" << Thread::current().threadId() << "] " <<
        msg;
}
    
Pool::Pool(int workerCount, int workerTaskMax) :
    _workerTaskMax(workerTaskMax)
{       
    for (auto i: range(workerCount)) { _workers.push_back(make_unique<Worker>(*this)); mt_unused(i); }
    for (auto& e: _workers) e->start();
}

Pool::~Pool()
{
    for (auto& e: _workers) e->join();
}

void Pool::enqueue_(TaskPtr task)
{    
    //Find smallest worker queue
    int minSize = _workerTaskMax;
    int minIndex = -1;
    for (auto i: range(size(_workers)))
    {
        auto& worker = *_workers[i];
        if (size(worker._tasks) >= minSize) continue;
        minSize = size(worker._tasks);
        minIndex = i;
    }
    
    bool added = false;
    if (minIndex >= 0)
    {
        //Push to worker queue
        do
        {
            Worker& worker = *_workers[minIndex];
            ConditionLock::Scoped _(worker._cond);
            if (size(worker._tasks) >= _workerTaskMax) break;
            
            added = true;
            worker._tasks.push_back(move(task));
            Pool_trace(worker._tasks.back(), sout() << "Pushed to worker queue: " << worker._thread.threadId()
                                                    << "; Queue size: " << worker._tasks.size());
        } while(false);
    }
    
    if (!added)
    {
        //All worker queues full, push to pool queue
        Mutex::Scoped _(_lock);
        _tasks.push_back(move(task));
        Pool_trace(_tasks.back(), sout() << "Pushed to pool queue. Queue size: " << _tasks.size());
    }
    
    //Find a waiting worker and signal it, start search at min index
    int first = minIndex >= 0 ? minIndex : 0;
    for (auto i: range(size(_workers)))
    {
        Worker& worker = *_workers[(first + i) % size(_workers)];
        ConditionLock::Scoped _(worker._cond);
        if (!worker._condWait) continue;
        worker._condWait = false;
        worker._cond.signal();
        break;
    }
}

thread::Local<typename Pool::Worker*> Pool::Worker::_current;

Pool::Worker::Worker(Pool& pool) :
    _pool(pool),
    _thread(honey::bind(&Worker::run, this)),
    _active(false),
    _condWait(false),
    _task(nullptr)
{
}

void Pool::Worker::start()
{
    _thread.start();
    //Synchronize with thread
    while (!_active) { ConditionLock::Scoped _(_cond); }
}

void Pool::Worker::join()
{
    {
        ConditionLock::Scoped _(_cond);
        _active = false;
        _condWait = false;
        _cond.signal();
    }
    _thread.join();
}

void Pool::Worker::run()
{
    {
        ConditionLock::Scoped _(_cond);
        _current = this;
        _active = true;
        _condWait = true;
    }
    
    while (_active)
    {
        while ((_task = next())) _task();
        
        //Wait for a task to be queued (ignore any thread interrupts)
        ConditionLock::Scoped _(_cond);
        while (_condWait) try { _cond.wait(); } catch (...) {}
        _condWait = true;
    }
}

auto Pool::Worker::next() -> TaskPtr
{
    //Try to pop from our queue
    {
        ConditionLock::Scoped _(_cond);
        if (_tasks.size())
        {
            TaskPtr task = move(_tasks.front());
            _tasks.pop_front();
            Pool_trace(task, sout() << "Popped from worker queue. Queue size: " << _tasks.size());
            return task;
        }
    }
    
    //Find largest other worker queue
    int maxSize = 0;
    int maxIndex = -1;
    for (auto i: range(size(_pool._workers)))
    {
        Worker& worker = *_pool._workers[i];
        if (size(worker._tasks) <= maxSize) continue;
        maxSize = size(worker._tasks);
        maxIndex = i;
    }
    
    if (maxIndex >= 0)
    {
        //Steal from other worker queue
        do
        {
            Worker& worker = *_pool._workers[maxIndex];
            ConditionLock::Scoped _(worker._cond);
            if (!worker._tasks.size()) break;
            
            TaskPtr task = move(worker._tasks.front());
            worker._tasks.pop_front();
            Pool_trace(task, sout() << "Stolen from worker queue: " << worker._thread.threadId()
                                    << "; Queue size: " << worker._tasks.size());
            return task;
        } while(false);
    }
        
    //Pop task from pool queue
    if (_pool._tasks.size())
    {
        do
        {
            Mutex::Scoped _(_pool._lock);
            if (!_pool._tasks.size()) break;
        
            TaskPtr task = move(_pool._tasks.front());
            _pool._tasks.pop_front();
            Pool_trace(task, sout() << "Popped from pool queue. Queue size: " << _pool._tasks.size());
            return task;
        } while (false);
    }
    
    return nullptr;
}

} }




