// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Lock/SharedMutex.h"
#include "Honey/Thread/Thread.h"

namespace honey
{

void SharedMutex::init()
{
    _state = State::unlock;
    _owner = Thread::threadIdInvalid;
    _holdCount = 0;
    _waitCount = 0;
}

void SharedMutex::lock()
{
    Thread::ThreadId threadId = Thread::current().threadId();
    if (tryLock_priv(threadId)) return;
    //Atomic lock failed, wait on condition
    ConditionLock::Scoped _(_cond);
    ++_waitCount;
    while (!tryLock_priv(threadId)) _cond.wait();
    --_waitCount;
}

void SharedMutex::unlock()
{
    assert(_owner == Thread::current().threadId() && _holdCount > 0, "Unlock Error: Thread does not hold the unique lock.");
    if (--_holdCount > 0) return;
    _owner = Thread::threadIdInvalid;
    _state = State::unlock;
    //Notify unique and shared waiters that the unique lock is released
    if (_waitCount == 0) return;
    ConditionLock::Scoped _(_cond);
    _cond.broadcast();
}

inline bool SharedMutex::tryLock_priv(Thread::ThreadId threadId)
{
    if (_owner != threadId && !_state.cas(State::unique, State::unlock)) return false;
    _owner = threadId;
    ++_holdCount;
    return true;
}

bool SharedMutex::tryLock()
{
    return tryLock_priv(Thread::current().threadId());
}

bool SharedMutex::tryLock(MonoClock::TimePoint time)
{
    Thread::ThreadId threadId = Thread::current().threadId();
    if (tryLock_priv(threadId)) return true;
    //Atomic lock failed, wait on condition
    ConditionLock::Scoped _(_cond);
    bool res;
    ++_waitCount;
    while ((res = tryLock_priv(threadId)) == false && _cond.wait(time));
    --_waitCount;
    return res;
}

void SharedMutex::lockShared()
{
    if (tryLockShared_priv()) return;
    //Atomic lock failed, wait on condition
    ConditionLock::Scoped _(_cond);
    ++_waitCount;
    while (!tryLockShared_priv()) _cond.wait();
    --_waitCount;
}

void SharedMutex::unlockShared()
{
    assert(isShared(), "Unlock Error: Thread does not hold a shared lock.");
    _state -= State::shared;
    if (isShared()) return;
    //Notify unique waiters that the shared lock is released
    if (_waitCount == 0) return;
    ConditionLock::Scoped _(_cond);
    _cond.signal();
}

inline bool SharedMutex::tryLockShared_priv()
{
    int old;
    do 
    {
        old = _state;
        if (old == State::unique) return false;
    } while (!_state.cas(old + State::shared, old));
    return true;
}

bool SharedMutex::tryLockShared()
{
    return tryLockShared_priv();
}

bool SharedMutex::tryLockShared(MonoClock::TimePoint time)
{
    if (tryLockShared_priv()) return true;
    //Atomic lock failed, wait on condition
    ConditionLock::Scoped _(_cond);
    bool res;
    ++_waitCount;
    while ((res = tryLockShared_priv()) == false && _cond.wait(time));
    --_waitCount;
    return res;
}

void SharedMutex::unlockAndLockShared()
{
    assert(_owner == Thread::current().threadId() && _holdCount > 0, "Unlock Error: Thread does not hold the unique lock.");
    assert(_holdCount == 1, "Unlock Error: Thread still holds recursive unique locks.");
    //Unlock unique and lock shared
    --_holdCount;
    _owner = Thread::threadIdInvalid;
    _state = State::shared;
    //Notify shared waiters that the unique lock is released
    if (_waitCount == 0) return;
    ConditionLock::Scoped _(_cond);
    _cond.broadcast();
}

}
