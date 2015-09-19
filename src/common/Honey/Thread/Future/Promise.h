// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Exception.h"
#include "Honey/Thread/Condition/Lock.h"

namespace honey
{

/// Future util
namespace future
{
    /// Exceptions
    struct Broken : Exception                                   { EXCEPTION(Broken) };
    struct FutureAlreadyRetrieved : Exception                   { EXCEPTION(FutureAlreadyRetrieved) };
    struct AlreadySatisfied : Exception                         { EXCEPTION(AlreadySatisfied) };
    struct NoState : Exception                                  { EXCEPTION(NoState) };

    /** \cond */
    namespace priv
    {
        struct StateBase : SharedObj<StateBase>
        {
            template<class Alloc>
            StateBase(Alloc&& a)                                : SharedObj(forward<Alloc>(a)), ready(false), futureRetrieved(false) {}

            virtual ~StateBase()
            {
                // onReady functors must delete themselves
                if (!ready) for (auto& e : onReady) e(*this);
            }
            
            void setException(const Exception::ConstPtr& e, bool setReady = true)
            {
                ConditionLock::Scoped _(waiters);
                if (ready) throw_ AlreadySatisfied();
                ex = e;
                if (setReady) setReady_();
            }

            void setReady()                                     { ConditionLock::Scoped _(waiters); setReady_(); }
            
            template<class Func>
            void addOnReady(Func&& f)
            {
                SharedPtr<StateBase> this_;
                ConditionLock::Scoped _(waiters);
                if (ready)
                {
                    this_ = this; //functor may own this state and delete itself, delay deletion until lock is released
                    f(*this);
                }
                else
                    onReady.push_back(mt::FuncptrCreate(forward<Func>(f)));
            }
            
            Exception::ConstPtr ex;
            bool ready;
            bool futureRetrieved;
            ConditionLock waiters;
            //std::function is not used here to avoid the operator() virtual call.
            //Functors must delete themselves after running.  If src.ready is false then src is being destructed.
            vector<mt::Funcptr<void (StateBase& src)>> onReady;
            
        protected:
            void setReady_()
            {
                assert(!ready);
                ready = true;
                waiters.broadcast();
                for (auto& e : onReady) e(*this);
            }
        };

        /// State with generic result
        template<class R>
        struct State : StateBase
        {
            template<class Alloc>
            State(Alloc&& a)                                    : StateBase(forward<Alloc>(a)) debug_if(, _result(&result())) {}

            ~State()
            {
                //Result uses generic storage, must destroy manually
                if (ready && !ex) result().~R();
            }
            
            template<class T>
            void setValue(T&& val, bool setReady = true)
            {
                ConditionLock::Scoped _(waiters);
                if (ready) throw_ AlreadySatisfied();
                new (&_storage) R(forward<T>(val));
                if (setReady) setReady_();
            }

            R& result()                                         { return reinterpret_cast<R&>(_storage); }
            const R& result() const                             { return reinterpret_cast<const R&>(_storage); }
            
            /// Use generic storage so result is not constructed until needed
            typedef typename std::aligned_storage<sizeof(R), alignof(R)>::type Storage;
            Storage _storage;
            debug_if(R* _result;) //make result visible in debugger
        };

        /// State with reference result
        template<class R>
        struct State<R&> : StateBase
        {
            template<class Alloc>
            State(Alloc&& a)                                    : StateBase(forward<Alloc>(a)), _result(nullptr) {}

            template<class T>
            void setValue(T& val, bool setReady = true)
            {
                ConditionLock::Scoped _(waiters);
                if (ready) throw_ AlreadySatisfied();
                _result = &val;
                if (setReady) setReady_();
            }

            R& result()                                         { assert(_result); return *_result; }
            const R& result() const                             { assert(_result); return *_result; }
            
            R* _result;
        };

        /// State with void result
        template<>
        struct State<void> : StateBase
        {
            template<class Alloc>
            State(Alloc&& a)                                    : StateBase(forward<Alloc>(a)) {}
            
            void setValue(bool setReady = true)
            {
                ConditionLock::Scoped _(waiters);
                if (ready) throw_ AlreadySatisfied();
                if (setReady) setReady_();
            }
        };
        
        /// Invoke function with args and set result into state
        template<class R>
        struct invoke
        {
            typedef State<R> State;

            template<class F, class... Args>
            void operator()(State& state, bool setReady, F&& f, Args&&... args)
            {
                try { state.setValue(f(forward<Args>(args)...), setReady); }
                catch (...) { state.setException(Exception::current(), setReady); }
            }
        };

        /// Void result
        template<>
        struct invoke<void>
        {
            typedef State<void> State;

            template<class F, class... Args>
            void operator()(State& state, bool setReady, F&& f, Args&&... args)
            {
                try { f(forward<Args>(args)...); state.setValue(setReady); }
                catch (...) { state.setException(Exception::current(), setReady); }
            }
        };
    }
    /** \endcond */
}

template<class R> class Future;

/// Container to hold a delayed function result
/**
  * A promise must be fulfilled before being destroyed, otherwise its future will return the exception future::Broken.
  */
template<class R>
class Promise : mt::NoCopy
{
    template<class R_> friend class Future;
    template<class Sig> friend class PackagedTask;
public:
    typedef future::priv::State<R> State;
    
    /// Construct with allocator for shared state
    template<class Alloc = std::allocator<State>>
    Promise(Alloc&& a = Alloc())
    {
        typedef typename mt::removeRef<Alloc>::type::template rebind<State>::other Alloc_;
        Alloc_ a_ = forward<Alloc>(a);
        _state.set(new (a_.allocate(1)) State(a_));
    }
    
    Promise(Promise&& rhs) noexcept                             : _state(nullptr) { operator=(move(rhs)); }
    ~Promise()                                                  { finalize(); }

    Promise& operator=(Promise&& rhs)                           { finalize(); _state = move(rhs._state); return *this; }

    /// Get future from which delayed result can be retrieved.
    /**
      * \throws future::FutureAlreadyRetrieved      if future() has been called more than once.
      * \throws future::NoState                     if invalid
      */ 
    Future<R> future()
    {
        if (!valid()) throw_ future::NoState();
        ConditionLock::Scoped _(_state->waiters);
        if (_state->futureRetrieved) throw_ future::FutureAlreadyRetrieved();
        _state->futureRetrieved = true;
        return Future<R>(_state);
    }

    /// Set stored result.  Result is copy/move constructed from value.
    /**
      * \throws future::AlreadySatisfied    if a result has already been set
      * \throws future::NoState             if invalid
      */ 
    template<class T>
    typename mt::disable_if<mt::True<T>::value && (mt::isRef<R>::value || std::is_void<R>::value)>::type
        setValue(T&& val)                                       { if (!valid()) throw_ future::NoState(); _state->setValue(forward<T>(val)); }
    /// Set stored result for ref result type
    template<class T>
    typename std::enable_if<mt::True<T>::value && mt::isRef<R>::value>::type
        setValue(T& val)                                        { if (!valid()) throw_ future::NoState(); _state->setValue(val); }
    /// Set stored result for void result type
    void setValue()                                             { static_assert(!mt::True<R>::value, "Only for use with void type"); }

    /// Set stored exception. Exception must be heap allocated.
    /**
      * \throws future::AlreadySatisfied    if a result has already been set
      * \throws future::NoState             if invalid
      */ 
    void setException(const Exception::ConstPtr& e)             { if (!valid()) throw_ future::NoState(); _state->setException(e); }

    /// Check if this instance has state and can be used.  State can be transferred out to another instance through move-assignment.
    bool valid() const                                          { return _state; }

    /// Get the shared state
    State& __state() const                                      { assert(_state); return *_state; }
    
private:
    explicit Promise(const SharedPtr<State>& state)             : _state(state) {}
    
    void finalize()                                             { if (valid() && !_state->ready) setException(new future::Broken()); }

    SharedPtr<State> _state;
};

template<> inline void Promise<void>::setValue()                { if (!valid()) throw_ future::NoState(); _state->setValue(); }

}
