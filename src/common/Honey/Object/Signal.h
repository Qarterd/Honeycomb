// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Id.h"
#include "Honey/Misc/StdUtil.h"
#include "Honey/Memory/SmallAllocator.h"

namespace honey
{

/**
  * \defgroup Signal    Signals and Listeners
  *
  * Signals and listeners provide a way to broadcast one function call to multiple callees.
  */
/// @{

/// Call once inside a class that has signals
#define SIGNAL_DECL(BaseClass) \
    static mt_global(const String, _signalBase, (#BaseClass));

/// Call inside a class to declare a signal
#define SIGNAL(Name, Param) \
    struct Name                                 : Signal<void Param> { static mt_global(const Id, id, (sout() << _signalBase() << "::"#Name)); };

/// Multicast sender
template<class Sig_> struct Signal              : mt::funcTraits<Sig_> { typedef Sig_ Sig; };

/// Multicast receiver
class SlotBase : public SmallAllocatorObject
{
public:
    SlotBase(const Id& id, const Id& signalId)  : _id(id), _signalId(signalId) {}
    virtual ~SlotBase() {}

    const Id& id() const                        { return _id; }
    const Id& signalId() const                  { return _signalId; }
private:
    Id _id;
    Id _signalId;
};

/** \cond */
namespace priv
{
    template<class Signal, class Seq = mt::make_idxseq<Signal::arity>>
    struct SlotSignal;
    template<class Signal, size_t... Seq>
    struct SlotSignal<Signal,mt::idxseq<Seq...>> : SlotBase
    {
        SlotSignal(const Id& id)                : SlotBase(id, Signal::id()) {}
        virtual void operator()(const typename Signal::template param<Seq>&... args) = 0;
    };

    template<class Signal, class F, class Seq = mt::make_idxseq<Signal::arity>>
    struct Slot;
    template<class Signal, class F, size_t... Seq>
    struct Slot<Signal,F,mt::idxseq<Seq...>> : public SlotSignal<Signal>
    {
        typedef SlotSignal<Signal> Super;
    public:
        Slot(const Id& id, F&& f)               : Super(id), _f(forward<F>(f)) {}

        virtual void operator()(const typename Signal::template param<Seq>&... args)
        {
            _f(args...);
        }

    private:
        F _f;
    };
}
/** \endcond */

/// @}

}


