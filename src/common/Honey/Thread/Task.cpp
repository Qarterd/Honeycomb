// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Task.h"
#include "Honey/Misc/Log.h"

namespace honey
{

#ifndef FINAL
    #define Task_trace(task, msg)   if ((task).traceEnabled()) (task).trace(__FILE__, __LINE__, (msg));
#else
    #define Task_trace(...) {}
#endif
    
Task::Task(const Id& id) :
    _state(State::idle),
    _depNode(this, id),
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

Task& Task::current()
{
    Task* task = static_cast<Task*>(thread::Pool::current());
    assert(task, "No active task in current thread, this method can only be called from a task functor");
    return *task;
}
    
void Task::bindDirty()
{
    //If we are part of the root's binding, inform root that its subgraph is now dirty 
    auto root = _root.lock();
    if (root && _sched == root->_sched && _bindId == root->_bindId)
        root->_bindDirty = true;
}

void Task::operator()()
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
            Task_trace(*this, sout() << "Waiting for upstream. Wait task count: " << _depUpWait);
            return;
        }
        assert(!_depUpWait, "Task state corrupt");
        _state = State::exec;
        _thread = &Thread::current();
        if (_priority != Thread::priorityNormal()) _thread->setPriority(_priority);
    }
    
    Task_trace(*this, "Executing");
    try { exec(); } catch (std::exception& e) { Log_debug << info() << "Unexpected task execution error: " << e; }
    Task_trace(*this, "Completed");
    
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
        Task& e = ****vertex->nodes().begin();
        Mutex::Scoped _(e._lock);
        if (--e._depDownWait > 0) continue;
        e.finalize_();
    }
    
    //Re-enqueue any downstream tasks that are waiting
    for (auto& vertex: _vertex->links(DepNode::DepType::in))
    {
        if (!vertex->nodes().size()) continue;
        Task& e = ****vertex->nodes().begin();
        if (e._sched != _sched || e._bindId != _bindId) continue; //This task is not upstream of root
        {
            Mutex::Scoped _(e._lock);
            if (--e._depUpWait > 0) continue;
            if (e._state != State::depUpWait) continue;
        }
        _sched->enqueue_priv(e);
    }
    
    {
        Mutex::Scoped _(_lock);
        //Root task must finalize itself
        if (this == _root.lock())
        {
            --_depDownWait;
            finalize_();
            return;
        }
        //If we haven't been finalized yet then we must wait for downstream to finalize us
        if (_state != State::idle)
        {
            _state = State::depDownWait;
            Task_trace(*this, sout() << "Waiting for downstream. Wait task count: " << _depDownWait);
            return;
        }
    }
}

void Task::finalize_()
{
    //Reset task to initial state
    assert(!_depDownWait, "Task state corrupt");
    _depUpWait = _depUpWaitInit;
    _depDownWait = _depDownWaitInit;
    _state = State::idle;
    Task_trace(*this, "Finalized");
    resetFunctor(); //makes future ready, so task may be destroyed beyond this point
}

String Task::info() const
{
    return sout() << "[Task: " << getId() << ":" << Thread::current().threadId() << "] ";
}

void Task::trace(const String& file, int line, const String& msg) const
{
    Log::inst() << log::level::debug <<
        "[" << log::srcFilename(file) << ":" << line << "] " <<
        info() << msg;
}

bool TaskSched::trace = false;

TaskSched::TaskSched(thread::Pool& pool) :
    _pool(&pool),
    _bindId(0) {}

bool TaskSched::reg(Task& task)
{
    Mutex::Scoped _(_lock);
    if (_depGraph.vertex(task) || !_depGraph.add(task._depNode)) return false;
    ++task._regCount;
    //Structural change, must dirty newly linked tasks
    auto vertex = _depGraph.vertex(task);
    assert(vertex);
    for (auto i: range(Task::DepNode::DepType::valMax))
    {
        for (auto& v: vertex->links(Task::DepNode::DepType(i)))
        {
            if (!v->nodes().size()) continue;
            Task& e = ****v->nodes().begin();
            if (e._sched == this) e.bindDirty();
        }
    }
    return true;
}

bool TaskSched::unreg(Task& task)
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

void TaskSched::bind(Task& root)
{
    //Binding is a pre-calculation step to optimize worker runtime, we want to re-use these results across multiple enqueues.
    //The root must be dirtied if the structure of its subgraph changes
    Mutex::Scoped _(_lock);
    Task_trace(root, "Binding root and its upstream");
    //Cache the vertex for each task
    root._vertex = _depGraph.vertex(root);
    assert(root._vertex, "Bind failed: task must be registered before binding");
    //The bind id gives us a way to uniquely identify all tasks upstream of root, this is critical when workers are returning downstream.
    ++_bindId;

    _taskStack.clear();
    _taskStack.push_back(&root);
    while (!_taskStack.empty())
    {
        Task& task = *_taskStack.back();
        
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
        task._root = Task::Ptr(&root);
        task._bindId = _bindId;
        task._bindDirty = false;
        task._depDownWaitInit = 0;
        task._depDownWait = task._depDownWaitInit;
        task._onStack = true;

        #ifdef DEBUG
            auto stackTrace = [&]() -> String
            {
                unordered_set<Task*> unique;
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
                Task& e = ****vertex->nodes().begin();
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
            Task& e = ****vertex->nodes().begin();
            e._vertex = vertex;
            _taskStack.push_back(&e);
            ++task._depUpWaitInit;
        }
        task._depUpWait = task._depUpWaitInit;
    }
}

bool TaskSched::enqueue(Task& task)
{
    if (task.active()) return false;
    if (task._sched != this || task._root.lock() != &task || task._bindDirty)
        bind(task);
    return enqueue_priv(task);
}

bool TaskSched::enqueue_priv(Task& task)
{
    {
        Mutex::Scoped _(task._lock);
        switch (task._state)
        {
            case Task::State::idle:
                task._state = Task::State::queued;
                break;
            case Task::State::depUpWait:
                if (task._depUpWait > 0) return false;
                task._state = Task::State::queued;
                break;
            default:
                return false;
        }
    }
    
    _pool->enqueue(task);
    return true;
}

/** \cond */
namespace task { namespace priv
{
    void test()
    {
        //Prints a b c d e f g h i j
        std::map<Char, Task_<void>::Ptr> tasks;
        for (auto i: range(10))
        {
            String name = sout() << Char('a'+i);
            tasks[name[0]] = new Task_<void>([=]{ Log_debug << Task::current().info(); }, name);
        }
        
        tasks['j']->deps().add(*tasks['i']);
        tasks['i']->deps().add(*tasks['h']);
        tasks['h']->deps().add(*tasks['g']);
        tasks['g']->deps().add(*tasks['f']);
        tasks['f']->deps().add(*tasks['e']);
        tasks['e']->deps().add(*tasks['d']);
        tasks['d']->deps().add(*tasks['c']);
        tasks['c']->deps().add(*tasks['b']);
        tasks['b']->deps().add(*tasks['a']);
        
        TaskSched sched(future::AsyncSched::inst());
        for (auto& e: values(tasks)) sched.reg(*e);

        auto future = tasks['j']->future();
        sched.enqueue(*tasks['j']);
        future.wait();
    }
} }
/** \endcond */

}




