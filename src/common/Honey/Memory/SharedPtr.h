// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Thread/Atomic.h"
#include "Honey/Misc/Debug.h"

namespace honey
{

/// \addtogroup Memory
/// @{

/** \cond */
namespace priv
{
    struct SharedObj_tag {};
    
    /// Control block for shared pointer.  Holds strong/weak reference counts.
    class SharedControl
    {
    public:
        SharedControl()                                 : _count(0), _weakCount(1) {}

        /// Increase reference count by 1
        void ref()                                      { ++_count; }
        /// Increase reference count by 1 if count is not 0 (ie. if object is alive then lock it).  Returns true on success.
        bool refLock()                                  { int old; do { old = _count; if (old <= 0) return false; } while (!_count.cas(old+1,old)); return true; }
        /// Decrease reference count by 1. Finalizes when count is 0.
        void unref()                                    { if (--_count > 0) return; finalize(); }
        /// Get reference count
        int count() const                               { return _count; }
    
        /// Increase weak reference count by 1
        void refWeak()                                  { ++_weakCount; }
        /// Decrease weak reference count by 1.  Destroys when count is 0.
        void unrefWeak()                                { if (--_weakCount > 0) return; destroy(); }
        /// Get weak reference count
        int weakCount() const                           { return _weakCount; }
        
        virtual void finalize() = 0;
        virtual void destroy() = 0;
        
        atomic::Var<int> _count;
        atomic::Var<int> _weakCount;
    };
    
    /// Control block for non-intrusive pointers.  Holds pointer and calls finalizer.  Alloc is used to deallocate the control block.
    template<class T, class Fin, class Alloc_>
    struct SharedControl_ : SharedControl
    {
        typedef typename Alloc_::template rebind<SharedControl_>::other Alloc;
        SharedControl_(T* ptr, Fin&& f, Alloc a)        : _ptr(ptr), _fin(forward<Fin>(f)), _alloc(move(a)) {}
        virtual void finalize()                         { _fin(_ptr); unrefWeak(); }
        virtual void destroy()                          { auto a = move(_alloc); delete_(this, a); }
        
        T* _ptr;
        Fin _fin;
        Alloc _alloc;
    };
    
    /// Control pointer storage for non-intrusive pointers
    template<class Subclass, class T, bool isIntrusive = mt::is_base_of<SharedObj_tag, T>::value>
    struct SharedControlPtr
    {
        SharedControlPtr()                              : __control(nullptr) {}
        SharedControl* _control() const                 { return __control; }
        void _control(SharedControl* ptr)               { __control = ptr; }
        
        SharedControl* __control;
    };
    
    template<class T, class Alloc_> struct SharedControl_obj;
}
/** \endcond */

//====================================================
// SharedObj
//====================================================

/// Reference-counted object for intrusive shared pointers.
/**
  * When possible, objects pointed to by SharedPtr should inherit from SharedObj, this is an intrusive pointer. \n
  * Intrusive pointers are safer than non-intrusive pointers as the shared ptr can be cast to a raw pointer and back again without issue. \n
  * The same operation with a non-intrusive pointer would result in each shared ptr holding a separate reference count, which will cause a crash.
  *
  * A shared object will be destroyed when the last strong reference (SharedPtr) is released,
  * but it will not be deallocated until the last weak reference (WeakPtr) is released.
  *
  * A shared object does not have to be assigned to a shared ptr to be destroyed properly, it has a normal life cycle.
  */
template<class Subclass>
class SharedObj : public priv::SharedObj_tag, mt::NoCopy
{
    template<class, class, bool> friend struct priv::SharedControlPtr;
    template<class> friend class SharedPtr;
    
public:
    /// Construct with allocator that is called to deallocate this shared object when all references have been released.
    template<class Alloc = std::allocator<Subclass>>
    SharedObj(Alloc&& a = Alloc())
        debug_if(: __control(&_control()))
    {
        typedef typename mt::removeRef<Alloc>::type::template rebind<Subclass>::other Alloc_;
        new (&_controlStorage) Control(subc(), Alloc_(forward<Alloc>(a)));
    }

protected:
    /// Destroys object. Called when strong reference count reaches 0.  May be overridden to prevent destruction.
    void finalize()                                     { Control& control = _control(); subc().~Subclass(); control.unrefWeak(); }

private:
    /**
      * This object must be destroyed separately from its control to implement weak references.
      * Although this object gets destroyed, its memory is held until its control has also been destroyed.
      */
    struct Control : priv::SharedControl
    {
        template<class Alloc>
        Control(Subclass& obj, Alloc&& a)               : _obj(&obj), _dealloc([=](Subclass* p) mutable { a.deallocate(p,1); }) {}

        virtual void finalize()                         { _obj->finalize(); }
        
        virtual void destroy()
        {
            auto p = _obj;
            auto dealloc = move(_dealloc);
            this->~Control();
            dealloc(p);
        }

        Subclass* _obj;
        function<void (Subclass*)> _dealloc;
    };
    typedef typename std::aligned_storage<sizeof(Control), alignof(Control)>::type ControlStorage;

    Subclass& subc()                                    { return static_cast<Subclass&>(*this); }
    
    Control& _control() const                           { return const_cast<Control&>(reinterpret_cast<const Control&>(_controlStorage)); }
    
    ControlStorage _controlStorage;
    debug_if(Control* __control;) //make control visible in debugger
};
    
/** \cond */
namespace priv
{
    /// Control pointer storage for intrusive pointers
    template<class Subclass, class T>
    struct SharedControlPtr<Subclass, T, true>
    {
        SharedControl* _control() const                 { return subc()._ptr ? &subc()._ptr->SharedObj::_control() : nullptr; }
        void _control(SharedControl*) {}
    private:
        const Subclass& subc() const                    { return static_cast<const Subclass&>(*this); }
    };
}
/** \endcond */

//====================================================
// SharedPtr
//====================================================

template<class T, class Fin> class UniquePtr;
template<class T> class WeakPtr;
    
/// Combined intrusive/non-intrusive smart pointer.  Can reference and share any object automatically.
/**
  * Non-intrusive pointers use the finalizer and internal control block allocator supplied as arguments. \n
  * Intrusive pointers finalize with SharedObj::finalize() and don't require an internal control block allocator.
  *
  * \see SharedObj, WeakPtr
  */
template<class T>
class SharedPtr : priv::SharedControlPtr<SharedPtr<T>, T>
{
    typedef priv::SharedControlPtr<SharedPtr<T>, T> ControlPtr;
    friend ControlPtr;
    template<class> friend class SharedPtr;
    template<class> friend class WeakPtr;
    template<class, class> friend struct priv::SharedControl_obj;
    template<class T_, class U> friend SharedPtr<T_> static_pointer_cast(const SharedPtr<U>&);
    template<class T_, class U> friend SharedPtr<T_> dynamic_pointer_cast(const SharedPtr<U>&);
    template<class T_, class U> friend SharedPtr<T_> const_pointer_cast(const SharedPtr<U>&);
    
    static const bool isIntrusive                                   = mt::is_base_of<priv::SharedObj_tag, T>::value;
    
    template<class T_, bool = std::is_void<T_>::value> struct Ref_  { typedef T_& type; static type deref(T_* p) { return *p; } };
    template<class T_> struct Ref_<T_, true>                        { typedef T_ type;  static type deref(T_* p) {} };
    
public:
    typedef T Elem;
    typedef typename Ref_<Elem>::type Ref;
    
    SharedPtr()                                                     : _ptr(nullptr) {}
    SharedPtr(nullptr_t)                                            : _ptr(nullptr) {}
    /// Reference an object. For intrusive pointers only.
    template<class U, typename std::enable_if<mt::True<U>::value && isIntrusive, int>::type=0>
    SharedPtr(U* ptr)                                               : _ptr(nullptr) { set(ptr); }
    /// Reference an object with finalizer and internal control block allocator. For non-intrusive pointers only.
    /**
      * Finalizer is run when reference count reaches 0 (deletes object by default).
      */
    template<class U, class Fin = finalize<U>, class Alloc = typename DefaultAllocator<U>::type, typename mt::disable_if<mt::True<U>::value && isIntrusive, int>::type=0>
    SharedPtr(U* ptr, Fin&& f = Fin(), Alloc&& a = Alloc())         : _ptr(nullptr) { set(ptr, forward<Fin>(f), forward<Alloc>(a)); }
    /// Reference the object pointed to by another shared pointer
    SharedPtr(const SharedPtr& ptr)                                 : _ptr(nullptr) { set(ptr); }
    template<class U> SharedPtr(const SharedPtr<U>& ptr)            : _ptr(nullptr) { set(ptr); }
    /// Transfer ownership out of shared pointer, leaving it null
    SharedPtr(SharedPtr&& ptr)                                      : _ptr(nullptr) { set(move(ptr)); }
    template<class U> SharedPtr(SharedPtr<U>&& ptr)                 : _ptr(nullptr) { set(move(ptr)); }
    
    /// Lock a weak pointer to get access to its object.  Shared ptr will be null if the object has already been destroyed.
    template<class U> SharedPtr(const WeakPtr<U>& ptr) :
        _ptr(ptr._ptr)
    {
        this->_control(ptr._control());
        if (_ptr && !getControl().refLock()) _ptr = nullptr;
    }
    
    /// Transfer ownership out of unique pointer, leaving it null
    template<class U, class Fin> SharedPtr(UniquePtr<U,Fin>&& ptr)  : _ptr(nullptr) { operator=(move(ptr)); }

    ~SharedPtr()                                                    { set(nullptr); }

    /// Set the object referenced by this shared pointer. Non-intrusive pointers use default finalizer/allocator.
    template<class U>
    SharedPtr& operator=(U* rhs)                                    { set(rhs); return *this; }
    SharedPtr& operator=(nullptr_t)                                 { set(nullptr); return *this; }
    SharedPtr& operator=(const SharedPtr& rhs)                      { set(rhs); return *this; }
    template<class U>
    SharedPtr& operator=(const SharedPtr<U>& rhs)                   { set(rhs); return *this; }
    SharedPtr& operator=(SharedPtr&& rhs)                           { set(move(rhs)); return *this; }
    template<class U>
    SharedPtr& operator=(SharedPtr<U>&& rhs)                        { set(move(rhs)); return *this; }
    
    template<class U, class Fin, typename std::enable_if<mt::True<U>::value && isIntrusive, int>::type=0>
    SharedPtr& operator=(UniquePtr<U,Fin>&& rhs)                    { set(rhs.release()); return *this; }
    template<class U, class Fin, typename mt::disable_if<mt::True<U>::value && isIntrusive, int>::type=0> 
    SharedPtr& operator=(UniquePtr<U,Fin>&& rhs)                    { set(rhs.release(), move(rhs.finalizer())); return *this; }
    
    template<class U>
    bool operator==(const SharedPtr<U>& rhs) const                  { return get() == rhs.get(); }
    template<class U>
    bool operator!=(const SharedPtr<U>& rhs) const                  { return !operator==(rhs); }
    template<class U>
    bool operator< (const SharedPtr<U>& rhs) const                  { return std::less<typename std::common_type<T*,U*>::type>()(get(), rhs.get()); }
    template<class U>
    bool operator> (const SharedPtr<U>& rhs) const                  { return rhs.operator<(*this); }
    template<class U>
    bool operator<=(const SharedPtr<U>& rhs) const                  { return !operator>(rhs); }
    template<class U>
    bool operator>=(const SharedPtr<U>& rhs) const                  { return !operator<(rhs); }

    bool operator==(nullptr_t) const                                { return !get(); }
    bool operator!=(nullptr_t) const                                { return get(); }
    friend bool operator==(nullptr_t, const SharedPtr& rhs)         { return !rhs.get(); }
    friend bool operator!=(nullptr_t, const SharedPtr& rhs)         { return rhs.get(); }

    T* operator->() const                                           { assert(_ptr); return _ptr; }
    Ref operator*() const                                           { assert(_ptr); return Ref_<Elem>::deref(_ptr); }
    operator T*() const                                             { return _ptr; }

    /// Get the raw pointer to the object
    T* get() const                                                  { return _ptr; }

    /// Dereference the current object and reference a new object. For intrusive pointers only.
    template<class U, typename std::enable_if<mt::True<U>::value && isIntrusive, int>::type=0>
    void set(U* ptr)                                                { setControl(ptr, ptr ? &ptr->SharedObj::_control() : nullptr); }
    /// Dereference the current object and reference a new object, with finalizer and internal control block allocator. For non-intrusive pointers only.
    template<class U, class Fin = finalize<U>, class Alloc = typename DefaultAllocator<U>::type, typename mt::disable_if<mt::True<U>::value && isIntrusive, int>::type=0>
    void set(U* ptr, Fin&& f = Fin(), Alloc&& a = Alloc())
    {
        typedef priv::SharedControl_<U,Fin,Alloc> Control;
        typedef typename Alloc::template rebind<Control>::other Alloc_;
        Alloc_ a_ = forward<Alloc>(a);
        setControl(ptr, ptr ? new (a_.allocate(1)) Control(ptr, forward<Fin>(f), move(a_)) : nullptr);
    }
    void set(nullptr_t)                                             { set((T*)nullptr); }

    /// Get number of shared references to the object
    int refCount() const                                            { if (_ptr) return getControl().count(); return 0; }
    /// Check whether this is the only shared reference to the object
    bool unique() const                                             { return refCount() == 1; }

private:
    template<class U> void set(const SharedPtr<U>& rhs)             { setControl(rhs._ptr, rhs._control()); }
    template<class U> void set(SharedPtr<U>&& rhs)                  { moveControl(rhs._ptr, rhs._control()); rhs._ptr = nullptr; rhs._control(nullptr); }
    
    template<class U>
    void setControl(U* ptr, priv::SharedControl* control)
    {
        if (ptr) control->ref();
        T* oldPtr = _ptr; priv::SharedControl* oldControl = this->_control();
        _ptr = ptr; this->_control(control);
        if (oldPtr) oldControl->unref();
    }
    
    template<class U>
    void moveControl(U* ptr, priv::SharedControl* control)
    {
        if (_ptr && this->_control() != control)
            this->_control()->unref();
        _ptr = ptr; this->_control(control);
    }
    
    priv::SharedControl& getControl() const                         { assert(this->_control()); return *this->_control(); }

    T* _ptr;
};

/** \cond */
namespace priv
{    
    /// Support for non-intrusive alloc_shared()
    template<class T, class Alloc_>
    struct SharedControl_obj : SharedControl
    {
        typedef typename mt::removeRef<Alloc_>::type::template rebind<SharedControl_obj>::other Alloc;
        typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type Storage;
        
        template<class... Args>
        static SharedPtr<T> create(Alloc a, Args&&... args)
        {
            auto* control = new (a.allocate(1)) SharedControl_obj(move(a), forward<Args>(args)...);
            SharedPtr<T> ptr;
            ptr.setControl(&control->_obj(), control);
            return ptr;
        }
        
        template<class... Args>
        SharedControl_obj(Alloc&& a, Args&&... args)                : _alloc(move(a)) { new (&_storage) T(forward<Args>(args)...); }
        virtual void finalize()                                     { _obj().~T(); unrefWeak(); }
        virtual void destroy()                                      { auto a = move(_alloc); delete_(this, a); }
        T& _obj()                                                   { return reinterpret_cast<T&>(_storage); }

        Storage _storage;
        Alloc _alloc;
    };
}
/** \endcond */

/// Create a shared ptr to an object of type T constructed with args.  The object and the internal control block are allocated together in a single allocation.
/** \relates SharedPtr */
template<class T, class Alloc, class... Args, typename mt::disable_if<mt::is_base_of<priv::SharedObj_tag, T>::value, int>::type=0>
SharedPtr<T> alloc_shared(Alloc&& a, Args&&... args)                { return priv::SharedControl_obj<T,Alloc>::create(forward<Alloc>(a), forward<Args>(args)...); }
/// Specializaton for intrusive pointers, simply allocates T and constructs with args
/** \relates SharedPtr */
template<class T, class Alloc, class... Args, typename std::enable_if<mt::is_base_of<priv::SharedObj_tag, T>::value, int>::type=0>
SharedPtr<T> alloc_shared(Alloc&& a, Args&&... args)                { return SharedPtr<T>(new (a.allocate(1)) T(forward<Args>(args)...)); }
/// alloc_shared() using T::Allocator if available, otherwise std::allocator  
/** \relates SharedPtr */
template<class T, class... Args, class Alloc = typename DefaultAllocator<T>::type>
SharedPtr<T> make_shared(Args&&... args)                            { return alloc_shared<T>(Alloc(), forward<Args>(args)...); }

/// \relates SharedPtr
template<class T, class U>
SharedPtr<T> static_pointer_cast(const SharedPtr<U>& rhs)           { SharedPtr<T> ret; ret.setControl(static_cast<T*>(rhs._ptr), rhs._control()); return ret; }
/// \relates SharedPtr
template<class T, class U>
SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U>& rhs)          { SharedPtr<T> ret; ret.setControl(dynamic_cast<T*>(rhs._ptr), rhs._control()); return ret; }
/// \relates SharedPtr
template<class T, class U>
SharedPtr<T> const_pointer_cast(const SharedPtr<U>& rhs)            { SharedPtr<T> ret; ret.setControl(const_cast<T*>(rhs._ptr), rhs._control()); return ret; }

//====================================================
// WeakPtr
//====================================================

/// Point to a shared object without holding a reference.  The object is accessible through a lock, which prevents unexpected destruction.
/**
  * Weak pointers can be used to break cyclic references. \n
  * If an outside user releases a reference to one object that internally is also referenced by its members,
  * then the object will not be destroyed as the user expects.
  * This problem can be solved by replacing internal shared pointers with weak pointers until the cycle is broken.
  *
  * \see SharedPtr
  */
template<class T>
class WeakPtr : priv::SharedControlPtr<WeakPtr<T>, T>
{
    typedef priv::SharedControlPtr<WeakPtr<T>, T> ControlPtr;
    friend ControlPtr;
    template<class> friend class WeakPtr;
    template<class> friend class SharedPtr;

public:
    WeakPtr()                                                       : _ptr(nullptr) {}
    WeakPtr(nullptr_t)                                              : _ptr(nullptr) {}
    /// Must construct from a shared pointer
    template<class U>
    WeakPtr(const SharedPtr<U>& rhs)                                : _ptr(nullptr) { operator=(rhs); }
    WeakPtr(const WeakPtr& rhs)                                     : _ptr(nullptr) { operator=(rhs); }
    template<class U>
    WeakPtr(const WeakPtr<U>& rhs)                                  : _ptr(nullptr) { operator=(rhs); }

    ~WeakPtr()                                                      { if (_ptr) getControl().unrefWeak(); }

    WeakPtr& operator=(const WeakPtr& rhs)                          { setControl(rhs._ptr, rhs._control()); return *this; }
    template<class U>
    WeakPtr& operator=(const WeakPtr<U>& rhs)                       { setControl(rhs._ptr, rhs._control()); return *this; }
    template<class U>
    WeakPtr& operator=(const SharedPtr<U>& rhs)                     { setControl(rhs._ptr, rhs._control()); return *this; }
    WeakPtr& operator=(nullptr_t)                                   { set(nullptr); return *this; }

    /// Set to null, release reference
    void set(nullptr_t)                                             { setControl(static_cast<T*>(nullptr), nullptr); }

    /// Acquire access to object.  Shared ptr prevents object from being destroyed while in use.  Returns null if object has been destroyed.
    SharedPtr<T> lock() const                                       { return *this; }

    /// Get strong reference (SharedPtr) count 
    int refCount() const                                            { if (_ptr) return getControl().count(); return 0; }
    /// Check whether the object has already been destroyed
    bool expired() const                                            { return refCount() == 0; }

private:
    template<class U>
    void setControl(U* ptr, priv::SharedControl* control)
    {
        if (ptr) control->refWeak();
        T* oldPtr = _ptr; priv::SharedControl* oldControl = this->_control();
        _ptr = ptr; this->_control(control);
        if (oldPtr) oldControl->unrefWeak();     
    }
    
    priv::SharedControl& getControl() const                         { assert(this->_control()); return *this->_control(); }

    T* _ptr;
};

/// @}

}

/** \cond */
namespace std
{
    /// Allow class to be used as key in unordered containers
    template<class T>
    struct hash<honey::SharedPtr<T>>
    {
        size_t operator()(const honey::SharedPtr<T>& val) const     { return reinterpret_cast<size_t>(val.get()); };
    };
}
/** \endcond */
