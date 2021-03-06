// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop
#include "Honey/Thread/Thread.h"

/** \cond */
namespace honey
{

namespace thread { namespace platform
{
    bool LocalStore::init()
    {
        static bool initOnce = false;
        if (initOnce) return true;
        initOnce = true;

        _index = TlsAlloc();
        assert(_index != TLS_OUT_OF_INDEXES);        
        return true;
    }

    LocalStore& LocalStore::create(Thread& thread)
    {
        LocalStore& local = *new LocalStore();
        local.thread = &thread;
        verify(TlsSetValue(_index, &local));
        return local;
    }

    void LocalStore::destroy()
    {
        delete_(&inst());
        TlsSetValue(_index, nullptr);
    }

    LocalStore& LocalStore::inst()
    {
        static auto _ = init(); mt_unused(_);

        LocalStore* local = static_cast<LocalStore*>(TlsGetValue(_index));
        if (!local)
        {
            //Externally created thread (ex. Main)
            create(*Thread::createExt());
            local = static_cast<LocalStore*>(TlsGetValue(_index));
        }
        assert(local, "Thread local data not created");
        return *local;
    }

    int LocalStore::_index;
} }

namespace platform
{

Thread::Thread(bool external, int stackSize) :
    _handle(nullptr),
    _id(threadIdInvalid),
    _stackSize(stackSize)
{
    if (external)
    {
        //Thread already created externally
        //Get thread handle
        DuplicateHandle(
            GetCurrentProcess(),    // Source Process Handle.
            GetCurrentThread(),     // Source Handle to dup.
            GetCurrentProcess(),    // Target Process Handle.
            &_handle,               // Target Handle pointer.
            0,                      // Options flag.
            TRUE,                   // Inheritable flag
            DUPLICATE_SAME_ACCESS   // Options
            );
        _id = GetCurrentThreadId();
    }
}

//Subclass calls move assign
Thread::Thread(Thread&&)                        : _handle(nullptr) {}
Thread::~Thread()                               { finalize(); }

void Thread::finalize()
{
    if (_handle) verify(CloseHandle(_handle));
}

Thread& Thread::operator=(Thread&& rhs)
{
    finalize();
    _handle = move(rhs._handle); rhs._handle = nullptr;
    _id = move(rhs._id);
    _stackSize = move(rhs._stackSize);
    return *this;
}

Thread& Thread::current()                       { return *LocalStore::inst().thread; }

void Thread::start()
{
    _handle = CreateThread(0, _stackSize, &entry, static_cast<honey::Thread*>(this), 0, (DWORD*)&_id);
    assert(_handle != NULL);
}

void Thread::join()
{
    verify(WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0);
}

int Thread::priorityNormal()                    { return 0; }
int Thread::priorityMin()                       { return -2; }
int Thread::priorityMax()                       { return 2; }

void Thread::setPriority(int priority)          { verify(SetThreadPriority(_handle, priority)); }
int Thread::getPriority() const                 { return GetThreadPriority(_handle); }

DWORD WINAPI Thread::entry(void* arg)
{
    honey::Thread* thread = static_cast<honey::Thread*>(arg);
    assert(thread);

    LocalStore::create(*thread);
    thread->entry();
    LocalStore::destroy();
    
    return 0;
}

int Thread::concurrency_priv()                  { SYSTEM_INFO info; GetSystemInfo(&info); return info.dwNumberOfProcessors; }

Thread* Thread::createExt()                     { return new honey::Thread(true, 0); }

} }
/** \endcond */



