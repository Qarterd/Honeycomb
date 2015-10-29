// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Thread/DepTask.h"
#include "Honey/Misc/Log.h"

namespace honey
{

#ifndef FINAL
    #define DepTask_trace(task, msg)    if ((task).traceEnabled()) (task).trace(__FILE__, __LINE__, (msg));
#else
    #define DepTask_trace(...) {}
#endif
    
DepTask::DepTask(const Id& id) :
    _depNode(this, id),
    _state(State::idle),
    _regCount(0),
    _sched(nullptr),
    _root(nullptr),
    _bindId(0),
    _bindDirty(true),
    _depUpWaitInit(0),
    _depUpWait(0),
    _depDownWaitInit(0),
    _depDownWait(0),
    _vertex(nullptr),
    _onStack(false),
    _thread(nullptr),
    _priority(Thread::priorityNormal()) {}

DepTask& DepTask::current()
{
    DepTask* task = static_cast<DepTask*>(thread::Pool::current());
    assert(task, "No active task in current thread, this method can only be called from a task functor");
    return *task;
}
    
void DepTask::bindDirty()
{
    //If we are part of the root's binding, inform root that its subgraph is now dirty 
    auto root = _root.lock();
    if (root && _sched == root->_sched && _bindId == root->_bindId)
        root->_bindDirty = true;
}

void DepTask::operator()()
{
    //Enqueue upstream tasks
    for (auto& vertex: _vertex->links())
    {
        if (!vertex->nodes().size()) continue;
        _sched->enqueue_priv(****vertex->nodes().begin());
    }
    
    {
        Mutex::Scoped _(_lock);
        //If there is an upstream task then we must wait to start
        if (_depUpWait > 0)
        {
            _state = State::depUpWait;
            DepTask_trace(*this, sout() << "Waiting for upstream. Wait task count: " << _depUpWait);
            return;
        }
        assert(!_depUpWait, "Task state corrupt");
        _state = State::exec;
        _thread = &Thread::current();
        if (_priority != Thread::priorityNormal()) _thread->setPriority(_priority);
    }
    
    DepTask_trace(*this, "Executing");
    try { exec(); } catch (std::exception& e) { Log_debug << info() << "Unexpected task execution error: " << e; }
    DepTask_trace(*this, "Completed");
    
    {
        Mutex::Scoped _(_lock);
        //restore priority to ensure its task-locality
        if (_priority != Thread::priorityNormal()) _thread->setPriority(Thread::priorityNormal());
        //consume any set interrupt to ensure its task-locality
        try { thread::current::interruptPoint(); } catch (...) {}
        _thread = nullptr;
    }
    
    //Finalize any upstream tasks that are waiting
    for (auto& vertex: _vertex->links())
    {
        if (!vertex->nodes().size()) continue;
        DepTask& e = ****vertex->nodes().begin();
        if (--e._depDownWait > 0) continue;
        Mutex::Scoped _(e._lock);
        e.finalize_();
    }
    
    {
        Mutex::Scoped _(_lock);
        //Re-enqueue any downstream tasks that are waiting
        for (auto& vertex: _vertex->links(DepNode::DepType::in))
        {
            if (!vertex->nodes().size()) continue;
            DepTask& e = ****vertex->nodes().begin();
            if (e._sched != _sched || e._bindId != _bindId) continue; //This task is not upstream of root
            if (--e._depUpWait > 0) continue;
            //Within enqueue_priv here we hold locks for both us and downstream,
            //but deadlock is not possible because downstream never holds locks for both itself and upstream
            _sched->enqueue_priv(e);
        }

        if (this == _root.lock())
        {
            //Root task must finalize itself
            --_depDownWait;
            finalize_();
        }
        else
        {
            //We must wait for downstream to finalize us
            _state = State::depDownWait;
            DepTask_trace(*this, sout() << "Waiting for downstream. Wait task count: " << _depDownWait);
        }
    }
}

void DepTask::finalize_()
{
    //Reset task to initial state
    assert(!_depDownWait, "Task state corrupt");
    _depUpWait = _depUpWaitInit;
    _depDownWait = _depDownWaitInit;
    _state = State::idle;
    DepTask_trace(*this, "Finalized");
    resetFunctor(); //makes future ready, so task may be destroyed beyond this point
}

String DepTask::info() const
{
    return sout() << "[Task: " << getId() << ":" << Thread::current().threadId() << "] ";
}

void DepTask::trace(const String& file, int line, const String& msg) const
{
    Log::inst() << log::level::debug <<
        "[" << log::srcFilename(file) << ":" << line << "] " <<
        info() << msg;
}

bool DepSched::trace = false;

DepSched::DepSched(thread::Pool& pool) :
    _pool(&pool),
    _bindId(0) {}

bool DepSched::reg(DepTask& task)
{
    Mutex::Scoped _(_lock);
    if (_depGraph.vertex(task) || !_depGraph.add(task._depNode)) return false;
    ++task._regCount;
    //Structural change, must dirty newly linked tasks
    auto vertex = _depGraph.vertex(task);
    assert(vertex);
    for (auto i: range(DepTask::DepNode::depTypeMax))
    {
        for (auto& v: vertex->links(DepTask::DepNode::DepType(i)))
        {
            if (!v->nodes().size()) continue;
            DepTask& e = ****v->nodes().begin();
            if (e._sched == this) e.bindDirty();
        }
    }
    return true;
}

bool DepSched::unreg(DepTask& task)
{
    Mutex::Scoped _(_lock);
    if (!_depGraph.remove(task._depNode)) return false;
    --task._regCount;
    //Structural change, must dirty task root
    if (task._sched == this)
    {
        task.bindDirty();
        task._sched = nullptr;
        task._root = nullptr;
    }
    return true;
}

void DepSched::bind(DepTask& root)
{
    //Binding is a pre-calculation step to optimize worker runtime, we want to re-use these results across multiple enqueues.
    //The root must be dirtied if the structure of its subgraph changes
    Mutex::Scoped _(_lock);
    DepTask_trace(root, "Binding root and its upstream");
    //Cache the vertex for each task
    root._vertex = _depGraph.vertex(root);
    assert(root._vertex, "Bind failed: task must be registered before binding");
    //The bind id gives us a way to uniquely identify all tasks upstream of root, this is critical when workers are returning downstream.
    ++_bindId;

    _taskStack.clear();
    _taskStack.push_back(&root);
    while (!_taskStack.empty())
    {
        DepTask& task = *_taskStack.back();
        
        //If already visited
        if (task._sched == this && task._bindId == _bindId)
        {
            //We are referenced by another downstream neighbour
            ++task._depDownWaitInit;
            task._depDownWait = task._depDownWaitInit;
            task._onStack = false;
            _taskStack.pop_back();
            continue;
        }
        
        //Not visited, bind task
        task.bindDirty();
        task._sched = this;
        task._root = DepTask::Ptr(&root);
        task._bindId = _bindId;
        task._bindDirty = false;
        task._depDownWaitInit = 0;
        task._depDownWait = task._depDownWaitInit;
        task._onStack = true;

        #ifdef DEBUG
            auto stackTrace = [&]() -> String
            {
                unordered_set<DepTask*> unique;
                int count = 0;
                ostringstream os;
                for (auto& e: reversed(_taskStack))
                {
                    if (!e->_onStack || !unique.insert(e).second) continue;
                    os << count++ << ". " << e->getId() << endl;
                }
                return os;
            };
        
            //Validate upstream tasks
            for (auto& vertex: task._vertex->links())
            {
                if (!vertex->nodes().size()) continue;
                DepTask& e = ****vertex->nodes().begin();
                if (e.active())
                {
                    error_(sout()   << "Bind failed: Upstream task already active. "
                                    << "Task: " << e.getId() << "; Task's root: " << (e._root.lock() ? e._root.lock()->getId() : idnull) << endl
                                    << "Task stack:" << endl << stackTrace());
                }
                
                if (e._onStack)
                {
                    error_(sout()   << "Bind failed: Upstream cyclic dependency detected. "
                                    << "From task: " << task.getId() << "; To task: " << e.getId() << endl
                                    << "Task stack:" << endl << stackTrace());
                }
            }
        #endif
        
        //Bind upstream tasks
        task._depUpWaitInit = 0; //count upstream tasks
        for (auto& vertex: task._vertex->links())
        {
            if (!vertex->nodes().size()) continue;
            DepTask& e = ****vertex->nodes().begin();
            e._vertex = vertex;
            _taskStack.push_back(&e);
            ++task._depUpWaitInit;
        }
        task._depUpWait = task._depUpWaitInit;
    }
}

bool DepSched::enqueue(DepTask& task)
{
    if (task.active()) return false;
    if (task._sched != this || task._root.lock() != &task || task._bindDirty)
        bind(task);
    return enqueue_priv(task);
}

bool DepSched::enqueue_priv(DepTask& task)
{
    {
        Mutex::Scoped _(task._lock);
        switch (task._state)
        {
            case DepTask::State::idle:
                task._state = DepTask::State::queued;
                break;
            case DepTask::State::depUpWait:
                if (task._depUpWait > 0) return false;
                task._state = DepTask::State::queued;
                break;
            default:
                return false;
        }
    }
    
    _pool->enqueue(task);
    return true;
}

}




