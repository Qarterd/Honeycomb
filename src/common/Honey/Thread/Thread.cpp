// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Condition/Lock.h"
#include "Honey/Thread/Lock/Spin.h"
#include "Honey/Misc/Range.h"

namespace honey
{

namespace thread
{
    /** \cond */
    namespace priv
    {
        InterruptWait::InterruptWait(Thread& thread, Condition& cond, Mutex& mutex) :
            thread(thread)
        {
            SpinLock::Scoped _(*thread._lock);
            thread._interruptCond = &cond;
            thread._interruptMutex = &mutex;
        }

        InterruptWait::~InterruptWait()
        {
            SpinLock::Scoped _(*thread._lock);
            thread._interruptCond = nullptr;
            thread._interruptMutex = nullptr;
        }
    }
    /** \endcond */

    namespace current
    {
        void sleep(MonoClock::Duration time)        { sleep(time == time.max() ? MonoClock::TimePoint::max() : MonoClock::now() + time); }

        void sleep(MonoClock::TimePoint time)
        {
            Thread& thread = Thread::current();
            ConditionLock::Scoped _(*thread._sleepCond);
            while (thread._sleepCond->wait(time));
        }

        bool interruptEnabled()                     { return Thread::current()._interruptEnable; }

        void interruptPoint()
        {
            Thread& thread = Thread::current();
            if (!thread._interruptEnable) return;
            SpinLock::Scoped _(*thread._lock);
            if (!thread._interruptEx) return;
            auto e = thread._interruptEx;
            thread._interruptEx = nullptr;
            e->raise();
        }
    }

    InterruptEnable::InterruptEnable(bool enable) :
        thread(Thread::current()),
        saveState(thread._interruptEnable)
                                                    { thread._interruptEnable = enable; }

    InterruptEnable::~InterruptEnable()             { thread._interruptEnable = saveState; }
}

Thread::Static::Static() :
    storeCount(0),
    storeLock(new SpinLock)
{
}

Thread::Static::~Static()
{
}

const Thread::ThreadId Thread::threadIdInvalid      = Super::threadIdInvalid;

Thread::Thread(bool external, int stackSize) :
    Super(external, stackSize),
    _lock(new SpinLock),
    _started(external),
    _done(false),
    _doneCond(new ConditionLock),
    _sleepCond(new ConditionLock),
    _interruptEnable(true),
    _interruptCond(nullptr),
    _interruptMutex(nullptr) {}

Thread::Thread(const Entry& entry, int stackSize) :
    Thread(false, stackSize)
{
    _entry = entry;
}

Thread::Thread(Thread&& rhs) noexcept :
    Super(move(rhs)),
    _started(false)
{
    operator=(move(rhs));
}

Thread::~Thread()                                   { finalize(); }

void Thread::finalize()
{
    assert(!_started || _done, "Thread must be joined");
    for (auto& e : _stores) e.fin();
}

Thread& Thread::operator=(Thread&& rhs)
{
    finalize();
    Super::operator=(move(rhs));
    _entry = move(rhs._entry);
    _lock = move(rhs._lock);
    _started = move(rhs._started);
    _done = move(rhs._done);
    _doneCond = move(rhs._doneCond);
    _sleepCond = move(rhs._sleepCond);
    _interruptEnable = move(rhs._interruptEnable);
    _interruptEx = move(rhs._interruptEx);
    _interruptCond = move(rhs._interruptCond);
    _interruptMutex = move(rhs._interruptMutex);
    _stores = move(rhs._stores);
    return *this;
}

void Thread::start()
{
    assert(!_started, "Thread already started");
    _started = true;
    Super::start();
}

void Thread::entry()
{
    _entry();

    _doneCond->lock();
    _done = true;
    _doneCond->broadcast();
    _doneCond->unlock();
}

bool Thread::join(MonoClock::TimePoint time)
{
    //Wait for execution to complete
    ConditionLock::Scoped _(*_doneCond);
    while (!_done && _doneCond->wait(time));

    //Wait for system thread to complete
    if (_done) Super::join();
    return _done;
}

void Thread::interrupt(const Exception::ConstPtr& e)
{
    SpinLock::Scoped _(*_lock);
    _interruptEx = e;
    if (_interruptEnable && _interruptCond)
    {
        Mutex::Scoped lock(*_interruptMutex, lock::Op::tryLock);
        if (lock) _interruptCond->broadcast();
    }
}

bool Thread::interruptRequested() const             { SpinLock::Scoped _(*const_cast<Thread*>(this)->_lock); return _interruptEx; }

Thread::StoreId Thread::allocStore()
{
    Static& ms = getStatic();
    SpinLock::Scoped _(*ms.storeLock);

    if (ms.storeIds.size() == 0)
    {
        //Allocate more ids
        int nextCount = ms.storeCount*2 + 1;
        int newCount = nextCount - ms.storeCount;
        for (int i = 0; i < newCount; ++i)
            ms.storeIds.push_back(ms.storeCount + i);
        ms.storeCount = nextCount;
    }

    StoreId id = ms.storeIds.back();
    ms.storeIds.pop_back();
    return id;
}

void Thread::freeStore(StoreId id)
{
    Static& ms = getStatic();
    if (!ms.storeLock) return; //Static may be destructed at app exit
    SpinLock::Scoped _(*ms.storeLock);
    ++id.reclaim;
    ms.storeIds.push_back(id);
}

}




