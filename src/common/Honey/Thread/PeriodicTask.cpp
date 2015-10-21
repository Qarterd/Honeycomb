// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/PeriodicTask.h"
#include "Honey/Misc/Log.h"

namespace honey
{

#ifndef FINAL
    #define PeriodicTask_trace(task, msg)    if ((task).traceEnabled()) (task).trace(__FILE__, __LINE__, (msg));
#else
    #define PeriodicTask_trace(...) {}
#endif
    
PeriodicTask::PeriodicTask(PeriodicSched& sched, optional<MonoClock::Duration> period, optional<MonoClock::Duration> delay, const Id& id) :
    _sched(sched),
    _period(period),
    _delay(delay),
    _id(id),
    _state(State::idle),
    _cancelled(false),
    _thread(nullptr),
    _priority(Thread::priorityNormal()) {}

PeriodicTask& PeriodicTask::current()
{
    PeriodicTask* task = static_cast<PeriodicTask*>(thread::Pool::current());
    assert(task, "No active task in current thread, this method can only be called from a task functor");
    return *task;
}
    
void PeriodicTask::cancel()
{
    Mutex::Scoped _(_lock);
    if (_cancelled) return;
    _cancelled = true;
    _sched.remove(*this);
}

void PeriodicTask::operator()()
{
    Ptr _(move(_self)); //keep ourself alive in this scope
    
    {
        Mutex::Scoped _(_lock);
        _thread = &Thread::current();
        if (_priority != Thread::priorityNormal()) _thread->setPriority(_priority);
    }
    
    PeriodicTask_trace(*this, "Executing");
    try { exec(); } catch (std::exception& e) { Log_debug << info() << "Unexpected task execution error: " << e; }
    PeriodicTask_trace(*this, "Completed");
    
    {
        Mutex::Scoped _(_lock);
        //restore priority to ensure its task-locality
        if (_priority != Thread::priorityNormal()) _thread->setPriority(Thread::priorityNormal());
        //consume any set interrupt to ensure its task-locality
        try { thread::current::interruptPoint(); } catch (...) {}
        _thread = nullptr;
        
        _state = _period && !_cancelled ? State::wait : State::idle; //tell scheduler that task is ready for next exec
        resetFunctor(); //make future ready
    }
}

String PeriodicTask::info() const
{
    return sout() << "[Task: " <<
        (_id ? String(sout() << _id) : String(sout() << std::hex << reinterpret_cast<intptr_t>(this))) <<
        ":" << Thread::current().threadId() << "] ";
}

void PeriodicTask::trace(const String& file, int line, const String& msg) const
{
    Log::inst() << log::level::debug <<
        "[" << log::srcFilename(file) << ":" << line << "] " <<
        info() << msg;
}

bool PeriodicSched::trace = false;

PeriodicSched::PeriodicSched(thread::Pool& pool) :
    _pool(&pool),
    _thread(bind(&PeriodicSched::run, this)),
    _active(false),
    _condWait(false)
{
    _thread.start();
    //Synchronize with thread
    while (!_active) { ConditionLock::Scoped _(_cond); }
}

PeriodicSched::~PeriodicSched()
{
    {
        ConditionLock::Scoped _(_cond);
        _active = false;
        _condWait = false;
        _cond.signal();
    }
    _thread.join();
}

void PeriodicSched::add(PeriodicTask& task)
{
    task._due = MonoClock::now() + (task._delay ? *task._delay : task._period ? *task._period : MonoClock::Duration::zero);
    task._state = PeriodicTask::State::wait;
    
    ConditionLock::Scoped _(_cond);
    _actions.push_back(make_tuple(Action::add, &task));
    _condWait = false;
    _cond.signal();
}

void PeriodicSched::remove(PeriodicTask& task)
{
    ConditionLock::Scoped _(_cond);
    _actions.push_back(make_tuple(Action::remove, &task));
    _condWait = false;
    _cond.signal();
}

void PeriodicSched::run()
{
    {
        ConditionLock::Scoped _(_cond);
        _active = true;
        _condWait = true;
    }
    
    while (_active)
    {
        if (!_actions.empty())
        {
            ConditionLock::Scoped _(_cond);
            for (auto& e: _actions)
            {
                auto& task = get<1>(e);
                switch (get<0>(e))
                {
                case Action::add:
                    PeriodicTask_trace(*task, sout() << "Scheduled, due in " << Millisec(task->_due.load() - MonoClock::now()) << "ms");
                    _tasks.insert(make_pair(task->_due.load(), move(task)));
                    break;
                    
                case Action::remove:
                    {
                        auto it = stdutil::findVal(_tasks, task->_due.load(), task);
                        if (it != _tasks.end())
                        {
                            PeriodicTask_trace(*task, "Cancelled");
                            _tasks.erase(it);
                        }
                    }
                    break;
                }
            }
            _actions.clear();
        }
        
        for (auto it = _tasks.begin(); it != _tasks.end();)
        {
            auto& task = it->second;
            if (task->_due.load() > MonoClock::now()) break;
            if (task->_state != PeriodicTask::State::wait) { ++it; continue; }
            _due.push_back(move(task));
            it = _tasks.erase(it);
        }
        
        for (auto& e: _due)
        {
            e->_state = PeriodicTask::State::exec;
            if (e->_period)
            {
                e->_due = MonoClock::now() + *e->_period;
                PeriodicTask_trace(*e, sout() << "Scheduled, due in " << Millisec(e->_due.load() - MonoClock::now()) << "ms");
                _tasks.insert(make_pair(e->_due.load(), e));
            }
            e->_self = e; //keep task alive at least until it is done
            _pool->enqueue(*e);
        }
        _due.clear();
        
        {
            //Wait until next task is due or for an action to be queued (ignore any thread interrupts)
            ConditionLock::Scoped _(_cond);
            bool timeout = false;
            while (_condWait && !timeout)
                try { timeout = !_cond.wait(!_tasks.empty() ? _tasks.begin()->second->_due.load() : MonoClock::TimePoint::max); } catch (...) {}
            _condWait = true;
        }
    }
}

}




