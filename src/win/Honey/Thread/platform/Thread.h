// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

namespace thread
{
    namespace platform
    {
        struct LocalStore;
    }
    
    namespace current { namespace platform
    {
        inline void yield()                                     { SwitchToThread(); }
        inline void pause()                                     { YieldProcessor(); }
    } }
}

namespace platform
{

class Thread
{
    typedef thread::platform::LocalStore LocalStore;
    friend struct LocalStore;
public:
    Thread(bool external, int stackSize);
    Thread(Thread&& rhs);
    virtual ~Thread();

    Thread& operator=(Thread&& rhs);

    static Thread& current();

    void start();
    void join();

    static const int priorityNormal                             = 0;
    static const int priorityMin                                = -2;
    static const int priorityMax                                = 2;
    void setPriority(int priority);
    int getPriority() const;

    typedef int32 ThreadId;
    static const ThreadId threadIdInvalid                       = 0;
    ThreadId threadId() const                                   { return _id; }

    static int concurrency()                                    { static int ret = concurrency_priv(); return ret; }

private:
    void finalize();

    static Thread* createExt();
    static DWORD WINAPI entry(void* arg);
    static int concurrency_priv();

    HANDLE      _handle;
    ThreadId    _id;
    int         _stackSize;
};

} }
/** \endcond */