// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Lock/Mutex.h"

namespace honey 
{

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
    void interrupt(const Exception::ConstPtr& e);
    
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