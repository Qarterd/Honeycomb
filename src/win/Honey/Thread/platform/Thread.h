// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

namespace platform { class Thread; }

namespace thread
{
    namespace platform
    {
        /// Thread local store. Every thread has its own separate store, can be retrieved statically.
        struct LocalStore
        {
            typedef honey::platform::Thread Thread;

            /// Init for process
            static bool init();
            /// Create thread local store
            static LocalStore& create(Thread& thread);
            /// Destroy thread local store
            static void destroy();
            /// Get thread local store
            static LocalStore& inst();

            Thread* thread;

            static int _index;
        };
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

    static int priorityNormal();
    static int priorityMin();
    static int priorityMax();
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