// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Object/Listener.h"
#include "Honey/Thread/Lock/Spin.h"

namespace honey
{
/** \cond */
namespace priv
{
    struct SlotQueueBase
    {
        virtual void process() = 0;
        virtual void clear() = 0;
    };

    /// This class determines how args are stored in queue
    template<class T>
    struct SlotQueueArg
    {
        typedef T StorageType;
        template<class T2>
        static void store(T& lhs, T2&& rhs)     { lhs = forward<T2>(rhs); }
        static T& load(T& lhs)                  { return lhs; }
    };

    /// For refs, store arg as pointer
    template<class T>
    struct SlotQueueArg<T&>
    {
        typedef T* StorageType;
        template<class T2>
        static void store(T*& lhs, T2&& rhs)    { lhs = &rhs; }
        static T& load(T* lhs)                  { return *lhs; }
    };

    template<class Signal, class F, class Seq = mt::IntSeqGen<Signal::arity>>
    class SlotQueue;
    template<class Signal, class F, int... Seq>
    class SlotQueue<Signal,F,mt::IntSeq<Seq...>> : public priv::SlotSignal<Signal>, public SlotQueueBase
    {
        typedef priv::SlotSignal<Signal> Super;
    public:
        SlotQueue(const Id& id, F&& f)          : Super(id), _f(forward<F>(f)) {}

        virtual void operator()(const typename Signal::template param<Seq>&... args)
        {
            SpinLock::Scoped _(_lock);
            _args.push_back(Args());
            Args& queued = _args.back();
            mt_unpackEval(priv::SlotQueueArg<typename Signal::template param<Seq>>::store(get<Seq>(queued), args));
        }

        virtual void process()
        {
            SpinLock::Scoped _(_lock);
            for (auto& args : _args)
                _f(priv::SlotQueueArg<typename Signal::template param<Seq>>::load(get<Seq>(args))...);
            _args.clear();
        }

        virtual void clear()
        {
            SpinLock::Scoped _(_lock);
            _args.clear();
        }

    private:
        typedef tuple<typename priv::SlotQueueArg<typename Signal::template param<Seq>>::StorageType...> Args;
        
        F _f;
        vector<Args, SmallAllocator<Args>> _args;
        SpinLock _lock;
    };
}
/** \endcond */
//====================================================
// ListenerQueue
//====================================================

/// Listener that holds queued slot for delayed processing of signals.
/**
  * \ingroup Signal
  *
  * Signal args must be default constructible and assignable.
  */
class ListenerQueue : public Listener
{
public:
    typedef SharedPtr<ListenerQueue> Ptr;
    typedef SharedPtr<const ListenerQueue> ConstPtr;

    /// Construct with slot to receive `Signal` using function `F`.  The object instance and id are used together to identify this listener.
    template<class Signal, class F>
    static ListenerQueue& create(F&& f, const void* obj = nullptr, const Id& id = idnull)
    {
        return *new ListenerQueue(*new priv::SlotQueue<Signal,F>(id, forward<F>(f)), obj, id);
    }
    
    /// Dispatch all signals stored in queue, clears queue when done
    void process()                                          { _slot.process(); }
    /// Remove all signals stored in queue
    void clear()                                            { _slot.clear(); }
    
private:
    template<class SlotQueue>
    ListenerQueue(SlotQueue& slot, const void* obj, const Id& id) :
        Listener(slot, obj, id), _slot(slot) {};
    
    priv::SlotQueueBase& _slot;
};

}
