// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Object/Signal.h"

namespace honey
{

/// Holds a slot that can receive signals
/** \ingroup Signal */
class Listener : public SharedObj<Listener>, public SmallAllocatorObject
{
public:
    typedef SharedPtr<Listener> Ptr;
    typedef SharedPtr<const Listener> ConstPtr;

    /// Construct with slot to receive `Signal` using function `F`.  The object instance and id are used together to identify this listener.
    template<class Signal, class F>
    static Listener& create(F&& f, const void* obj = nullptr, const Id& id = idnull)
    {
        return *new Listener(*new priv::Slot<Signal,F>(id, forward<F>(f)), obj, id);
    }

    virtual ~Listener() {}
    
    /// Get slot
    SlotBase& slot() const                              { assert(_slot); return *_slot; }
    /// Get object instance for listener identification
    const void* obj() const                             { return _obj; }
    /// Get listener id
    const Id& id() const                                { return _id; }

protected:
    Listener(SlotBase& slot, const void* obj, const Id& id) :
        SharedObj(SmallAllocator<Listener>()),
        _slot(&slot), _obj(obj), _id(id) {};
    
    UniquePtr<SlotBase> _slot;
    const void* _obj;
    Id _id;
};

}
