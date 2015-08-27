// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Task.h"

namespace honey 
{
    
/// Top-level application methods
namespace app
{
    /// An application module, holds a task and its dependencies. \see ModuleRegistry
    struct Module : SharedObj<Module>
    {
        typedef SharedPtr<Module> Ptr;
        
        Module( const Id& id, const function<void ()>& f,
                const vector<Id>& outDeps = {}, const vector<Id>& inDeps = {});
        
        Task_<void>::Ptr task;
    };

    /// List of application modules.
    /**
      * Register a module statically in a source file by calling:
      *
      *     static void myFunc();
      *     static auto _ = app::ModuleRegistry::inst().reg(new app::Module("myName"_id, &myFunc, {"outDep"_id}, {"inDep"_id}));
      *
      * \see Module
      */
    struct ModuleRegistry
    {
        /// Get singleton
        static mt_global(ModuleRegistry, inst,);
        
        /// Register a module
        mt::Void reg(const Module::Ptr& module);
        
        unordered_map<Id, Module::Ptr> modules;
    };
}

/// Top-level application flow controller, provides entry point and run loop
class App : mt::NoCopy
{
public:
    enum class RunMode
    {
        term,
        run
    };
    
    /// Process terminated. Use this interrupt to exit the run loop.
    struct Terminated : Exception               { EXCEPTION(Terminated) };
    
    App();
    ~App();
    
    /// Application entry point, call from main()
    virtual void entry() final;
    
    /// Request an interrupt in the app's thread. Exception must be heap allocated.
    void interrupt(const Exception::Ptr& e);
    
    RunMode runMode() const                     { return _runMode; }

    /// Number of times per second to interrupt modules
    int _interruptFreq;
    
protected:
    /// Module run loop
    virtual void run();
    
private:
    Thread* _thread;
    Mutex _lock;
    atomic::Var<RunMode> _runMode;
};
    
}