// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/App/App.h"

namespace honey
{

namespace app
{
    Module::Module( const Id& id, const function<void ()>& f,
                    const vector<Id>& outDeps, const vector<Id>& inDeps) :
        task(new Task_<void>(f, id))
    {
        task->deps().add("root"_id, Task::DepNode::DepType::in);
        for (auto& e: outDeps) task->deps().add(e);
        for (auto& e: inDeps) task->deps().add(e, Task::DepNode::DepType::in);
    }

    mt::Void ModuleRegistry::reg(const Module::Ptr& module)
    {
        modules[module->task->getId()] = module;
        return mt::Void();
    }
}

static auto _ = app::ModuleRegistry::inst().reg(new app::Module("root"_id, []{}));

App::App() :
    _interruptFreq(30),
    _thread(nullptr),
    _runMode(RunMode::term) {}

App::~App() {}
    
void App::entry()
{
    {
        Mutex::Scoped _(_lock);
        _thread = &Thread::current();
    }
    
    _runMode = RunMode::run;
    run();
    
    {
        Mutex::Scoped _(_lock);
        _thread = nullptr;
    }
}

void App::interrupt(const Exception::Ptr& e)
{
    Mutex::Scoped _(_lock);
    if (!_thread) return;
    _thread->interrupt(e);
}

void App::run()
{
    for (auto& e: values(app::ModuleRegistry::inst().modules)) TaskSched::inst().reg(*e->task);
    TaskSched::inst().enqueue(*app::ModuleRegistry::inst().modules["root"_id]->task);
    
    vector<app::Module::Ptr> modules;
    vector<Future<void>> results;
    for (auto& e: values(app::ModuleRegistry::inst().modules))
    {
        modules.push_back(e);
        results.push_back(move(e->task->future()));
    }
    
    while (!modules.empty())
    {
        try
        {
            thread::current::interruptPoint();
            auto it = future::waitAny(results, _runMode == RunMode::run ? Millisec::max : Millisec(1000 / _interruptFreq));
            if (it != results.end())
            {
                try { it->get(); }
                catch (Terminated& e) {}
                catch (Exception& e)
                {
                    Log::inst() << log::level::critical << e;
                }
                modules.erase(modules.begin() + (it - results.begin()));
                results.erase(it);
            }
            switch (_runMode)
            {
            case RunMode::term:
                {
                    //interrupt remaining modules with terminate
                    for (auto& e: modules) e->task->interrupt(new Terminated());
                    break;
                }
            default: break;
            }
        }
        catch (Terminated& e)
        {
            if (_runMode != RunMode::term)
            {
                Log::inst() << log::level::info << "Terminating...";
                _runMode = RunMode::term;
            }
        }
    }
}

}
