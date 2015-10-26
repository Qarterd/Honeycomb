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
    
Pool::Pool(szt workerCount, szt workerTaskMax) :
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
    szt minSize = _workerTaskMax;
    sdt minIndex = -1;
    for (auto i: range(_workers.size()))
    {
        auto size = _workers[i]->_tasks.size();
        if (size >= minSize) continue;
        minSize = size;
        minIndex = i;
    }
    
    bool added = false;
    if (minIndex >= 0)
    {
        //Push to worker queue
        Worker& worker = *_workers[minIndex];
        if (worker._tasks.size() < _workerTaskMax)
        {
            added = true;
            Pool_trace(task, sout() << "Pushed to worker queue: " << worker._thread.threadId()
                                    << "; Queue size: " << worker._tasks.size()+1);
            worker._tasks.push(move(task));
        }
    }
    
    if (!added)
    {
        //All worker queues full, push to pool queue
        Pool_trace(task, sout() << "Pushed to pool queue. Queue size: " << _tasks.size()+1);
        _tasks.push(move(task));
    }
    
    //Find a waiting worker and signal it, start search at min index
    sdt first = minIndex >= 0 ? minIndex : 0;
    for (auto i: range(_workers.size()))
    {
        Worker& worker = *_workers[(first + i) % _workers.size()];
        SpinLock::Scoped one(worker._condOne, lock::Op::tryLock); //we only need one producer to signal consumer
        if (!one) continue;
        ConditionLock::Scoped _(worker._cond);
        if (!worker._condWait) continue;
        worker._condWait = false;
        worker._cond.signal();
        one.unlock(); //allow next producer in before consumer processes signal
        break;
    }
}

thread::Local<typename Pool::Worker*> Pool::Worker::_current;

Pool::Worker::Worker(Pool& pool) :
    _pool(pool),
    _thread(honey::bind(&Worker::run, this)),
    _active(false),
    _condWait(false),
    _tasks(pool._workerTaskMax),
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
        TaskPtr task;
        if (_tasks.pop(task))
        {
            Pool_trace(task, sout() << "Popped from worker queue. Queue size: " << _tasks.size());
            return task;
        }
    }
    
    //Find largest other worker queue
    szt maxSize = 0;
    sdt maxIndex = -1;
    for (auto i: range(_pool._workers.size()))
    {
        auto size = _pool._workers[i]->_tasks.size();
        if (size <= maxSize) continue;
        maxSize = size;
        maxIndex = i;
    }
    
    if (maxIndex >= 0)
    {
        //Steal from other worker queue
        Worker& worker = *_pool._workers[maxIndex];
        TaskPtr task;
        if (worker._tasks.pop(task))
        {
            Pool_trace(task, sout() << "Stolen from worker queue: " << worker._thread.threadId()
                                    << "; Queue size: " << worker._tasks.size());
            return task;
        }
    }
        
    //Pop task from pool queue
    {
        TaskPtr task;
        if (_pool._tasks.pop(task))
        {
            Pool_trace(task, sout() << "Popped from pool queue. Queue size: " << _pool._tasks.size());
            return task;
        }
    }

    return nullptr;
}

} }




