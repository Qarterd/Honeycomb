// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Exception.h"
#include "Honey/Misc/Clock.h"
#include "Honey/Thread/platform/Thread.h"

namespace honey
{

class Thread;
class Condition;
class Mutex;

/// Thread util
namespace thread
{
    /** \cond */
    namespace priv
    {
        struct StoreId
        {
            StoreId(int id = -1)                    : id(id), reclaim(0) {}
            int id;
            int reclaim;
        };

        struct Store
        {
            Store()                                 : ptr(nullptr), reclaim(-1), fin(bind(finalize<void>(), ptr)) {}
            void* ptr;
            int reclaim;
            function<void ()> fin;
        };

        /// Attempt to throw interrupt at start (ctor) and end (dtor) of Condition::wait()
        class InterruptWait : mt::NoCopy
        {
        public:
            InterruptWait(Condition& cond, Mutex& mutex);
            ~InterruptWait();
        private:
            void test();
            Thread& thread;
            bool enable;
        };
    }
    /** \endcond */

    /// Operations on current Thread
    namespace current
    {
        /// Give up this thread's time slice to allow other threads to execute.
        inline void yield()                         { platform::yield(); }
        /// Perform a no-op without giving up this thread's time slice.  This no-op momentarily frees resources for other concurrent threads.
        inline void pause()                         { platform::pause(); }
        /// Suspend this thread for an amount of time
        void sleep(MonoClock::Duration time);
        /// Suspend this thread until a specific time
        void sleep(MonoClock::TimePoint time);
        /// Suspend this thread momentarily without giving up its time slice.  The thread will pause `count` times.
        inline void spin(int count)                 { for (int i = 0; i < count; ++i) pause(); }
        /// Check whether interrupts are enabled for this thread
        bool interruptEnabled();
        /// Throw an exception if interrupt is enabled and has been requested in this thread.
        void interruptPoint();
    }

    /// Local thread storage. Multiple threads can access one Local object, but each thread will only see its own separate "local" instance.
    template<class T>
    class Local
    {
    public:
        typedef function<T* ()> Init;
        typedef function<void (T*)> Fin;

        /// Init / Finalize func is called once per thread to create/destroy local object instance
        Local(const Init& init = &initObj, const Fin& fin = finalize<T>());
        ~Local();

        /// Assign thread-local object to rhs
        Local& operator=(const T& rhs)              { get() = rhs; return *this; }

        operator const T&() const                   { return get(); }
        operator T&()                               { return get(); }
        const T* operator->() const                 { return &get(); }
        T* operator->()                             { return &get(); }
        const T& operator*() const                  { return get(); }
        T& operator*()                              { return get(); }

        const T& get() const                        { const_cast<Local*>(this)->get(); }

        /// Get the thread-local object
        T& get();

    private:
        static T* initObj()                         { return new T; }

        priv::StoreId _id;
        Init _init;
        Fin _fin;
    };

    /// Interrupted exception
    struct Interrupted : Exception                  { EXCEPTION(Interrupted) };

    /// Enable / disable interrupts in the current thread's scope.
    /**
      * Example:
      *
      *     //Interrupts enabled by default
      *     {
      *         thread::InterruptEnable _(false);   //Disable interrupts in this scope
      *     }
      *     //Interrupts re-enabled
      */
    struct InterruptEnable : mt::NoCopy
    {
        InterruptEnable(bool enable);
        ~InterruptEnable();
    private:
        Thread& thread;
        bool saveState;
    };
}


class ConditionLock;
class SpinLock;

/// Thread class
class Thread : platform::Thread, mt::NoCopy
{
    typedef platform::Thread Super;
    friend class platform::Thread;
    friend void thread::current::sleep(MonoClock::TimePoint time);
    friend bool thread::current::interruptEnabled();
    friend void thread::current::interruptPoint();
    template<class T> friend class thread::Local;
    friend struct thread::InterruptEnable;
    friend class thread::priv::InterruptWait;

public:
    typedef Super::ThreadId ThreadId;
    typedef function<void ()> Entry;

    /// Construct a thread
    /**
      * \param entry        Execution function for thread.
      * \param stackSize    Initial size of the thread stack in bytes. If 0 then default size is used.
      */
    Thread(const Entry& entry, int stackSize = 0);
    /// Thread is moveable
    Thread(Thread&& rhs) noexcept;
    virtual ~Thread();

    Thread& operator=(Thread&& rhs);

    /// Get the thread object of the calling thread
    static Thread& current()                        { return static_cast<Thread&>(Super::current()); }

    /// Begin execution of this thread. The entry function will be called.
    void start();

    /// Wait until thread execution is complete
    void join()                                     { join(MonoClock::TimePoint::max); }
    /// Try to join for an amount of time. Returns true if joined and thread execution is complete.
    bool join(MonoClock::Duration time)             { return join(MonoClock::now() + time); }
    /// Try to join until a specific time. Returns true if joined and thread execution is complete.
    bool join(MonoClock::TimePoint time);

    /// Request an interrupt in the thread. Exception must be heap allocated.
    /**
      * The thread will throw `e` the next time it waits in join(), current::sleep(), current::interruptPoint(), or Condition::wait().
      */ 
    void interrupt(const Exception& e = *new thread::Interrupted);
    /// Check whether an interrupt has been requested for the thread
    bool interruptRequested() const;

    /// \name Thread scheduling priority
    /// Higher priority threads are favored for scheduling and will execute more often.
    /// @{
    static const int priorityNormal;
    static const int priorityMin;
    static const int priorityMax;
    /// @}

    /// Set thread execution scheduling priority
    void setPriority(int priority)                  { Super::setPriority(priority); }
    /// Get thread execution scheduling priority
    int getPriority() const                         { return Super::getPriority(); }

    /// Invalid thread id
    static const ThreadId threadIdInvalid;
    /// Get the thread unique platform id
    ThreadId threadId() const                       { return Super::threadId(); }

    /// Get number of threads that can be executed concurrently on the device
    static int concurrency()                        { return Super::concurrency(); }

private:
    typedef thread::priv::StoreId StoreId;
    typedef thread::priv::Store Store;

    /// For externally created threads (eg. Main), creates a thread object to represent the calling thread
    Thread(bool external, int stackSize);
    void finalize();

    /// Thread execution entry from platform
    void entry();

    /// Allocate a global store id
    static StoreId allocStore();
    /// Free a global store id
    static void freeStore(StoreId id);
    /// Get the thread-local pointer at store id
    Store& store(const StoreId& id)                 { const_cast<Thread*>(this)->expandStore(id.id); return _stores[id.id]; } 
    /// Expand the store so it can hold index `id`
    void expandStore(int id)                        { if (id < (int)_stores.size()) return; _stores.resize(id*2+1); }

    Entry                       _entry;
    UniquePtr<SpinLock>         _lock;
    bool                        _started;
    bool                        _done;
    UniquePtr<ConditionLock>    _doneCond;
    UniquePtr<ConditionLock>    _sleepCond;

    bool                        _interruptEnable;
    Exception::ConstPtr         _interruptEx;
    Condition*                  _interruptCond;
    Mutex*                      _interruptMutex;

    vector<Store>               _stores;

    struct Static
    {
        Static();
        ~Static();

        vector<StoreId>         storeIds;
        int                     storeCount;
        UniquePtr<SpinLock>     storeLock;
    };
    mt_staticObj(Static, getStatic,);
};


namespace thread
{
    template<class T>
    Local<T>::Local(const Init& init, const Fin& fin)   : _id(Thread::allocStore()), _init(init), _fin(fin) {}
    
    template<class T>
    Local<T>::~Local()                              { Thread::freeStore(_id); }
    
    template<class T>
    T& Local<T>::get()
    {
        priv::Store& store = Thread::current().store(_id);
        //This thread may not have init the ptr yet, or the ptr could be from an old local that has been deleted
        if (store.reclaim != _id.reclaim)
        {
            store.fin();
            store.reclaim = _id.reclaim;
            T* ptr = _init();
            store.ptr = ptr;
            //store.fin = std::bind(_fin, ptr); //TODO: clang bug, uncomment to properly destroy thread locals
            store.fin = std::bind(finalize<T>(), ptr);
        }
        return *reinterpret_cast<T*>(store.ptr);
    }
}

}
