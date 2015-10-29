// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Debug.h"

namespace honey
{

/// Null optional type. \see optnull.
struct optnull_t {};
/// Null optional, use to reset an optional to an uninitialized state or test for initialization
static optnull_t optnull;

/// Enables any type to be optional so it can exist in an uninitialized null state.
/**
    An optional is large enough to hold an instance of its wrapped type.
    On construction or first assignment the instance is copy/move constructed, assignments thereafter use copy/move assign.
    Assigning to `optnull` will reset the optional to an uninitialized state, destructing any instance.
 
    Supports wrapped const/ref types. \see optional<T&> for wrapped ref types.
    
    Variables are commonly defined as pointers for the sole reason that pointers can exist in a null state.
    In this case, an optional can be used instead of a pointer to make the behavior explicit:
 
        func(iterator* optIter = nullptr);  func(&iter)     ---->  func(optional<iterator> optIter = optnull);  func(iter)
        func(int* retVal = nullptr);        func(&retInt)   ---->  func(optional<int&> retVal = optnull);       func(retInt)
 
    \see optnull

    Example:
 
    \code
 
    func(optional<char> o = optnull)
    {
        optional<int&> rInt;
        int i, j;
        if (o == optnull) o = 'a';      //Set a default value if caller didn't specify a char
        rInt.bind(i);                   //Bind the reference before use
        rInt = 2;                       //Assignment to bound reference: i = 2
        rInt.bind(j);                   //Rebind to j
        rInt = o;                       //j = the wrapped char in 'o'
        rInt = optnull;                 //Reset to null unbound reference
        int a = *o;                     //The * and -> operators can be used to retrieve the wrapped object
        a = o + 1;                      //Optionals implicitly convert to their wrapped object
    }
 
    \endcode
  */
template<class T>
class optional
{
    template<class T_> friend class optional;
public:
    /// Uninitialized by default
    optional()                                      : _val(nullptr) {}
    optional(optnull_t)                             : _val(nullptr) {}
    optional(const optional& rhs)                   : _val(nullptr) { if (rhs._val) construct(rhs.get()); }
    optional(optional& rhs)                         : _val(nullptr) { if (rhs._val) construct(rhs.get()); }
    optional(optional&& rhs)                        : _val(nullptr) { if (rhs._val) construct(move(rhs.get())); }
    template<class U> optional(const optional<U>& rhs)  : _val(nullptr) { if (rhs._val) construct(rhs.get()); }
    template<class U> optional(optional<U>& rhs)    : _val(nullptr) { if (rhs._val) construct(rhs.get()); }
    template<class U> optional(optional<U>&& rhs)   : _val(nullptr) { if (rhs._val) construct(move(rhs.get())); }
    template<class U> optional(U&& rhs)             : _val(nullptr) { construct(forward<U>(rhs)); }

    ~optional()                                     { uninit(); }

    /// Reset the optional to an uninitialized state
    optional& operator=(optnull_t)                  { uninit(); return *this; }

    /// Assign wrapped object
    optional& operator=(const optional& rhs)        { return operator=<T>(rhs); }
    optional& operator=(optional& rhs)              { return operator=<T>(rhs); }
    optional& operator=(optional&& rhs)             { return operator=<T>(move(rhs)); }
    template<class U> optional& operator=(const optional<U>& rhs)   { if (rhs._val) operator=(rhs.get()); else uninit(); return *this; }
    template<class U> optional& operator=(optional<U>& rhs)         { if (rhs._val) operator=(rhs.get()); else uninit(); return *this; }
    template<class U> optional& operator=(optional<U>&& rhs)        { if (rhs._val) operator=(move(rhs.get())); else uninit(); return *this; }
    
    /// Assign object.  On the first assignment the instance is copy/move constructed, assignments thereafter use copy/move assign.
    /**
      * For an optional to be assignable to `rhs` it must also be constructible with `rhs`, this is necessary for allowing the optional to be nullable.
      */
    template<class U> optional& operator=(U&& rhs)  { if (!_val) construct(forward<U>(rhs)); else get() = forward<U>(rhs); return *this; }

    bool operator==(optnull_t) const                { return !_val; }
    bool operator!=(optnull_t) const                { return !operator==(optnull); }

    const T& operator*() const                      { return get(); }
    T& operator*()                                  { return get(); }
    const T* operator->() const                     { return &get(); }
    T* operator->()                                 { return &get(); }
    operator const T&() const                       { return get(); }
    operator T&()                                   { return get(); }

    /// Test whether optional is initialized
    explicit operator bool() const                  { return _val; }
    explicit operator bool()                        { return _val; }

    /// Get wrapped object. Optional must be initialized.
    const T& get() const                            { assert(_val, "Optional not initialized"); return *_val; }
    T& get()                                        { assert(_val, "Optional not initialized"); return *_val; }
    
    /// Get pointer to wrapped object. Returns null if not initialized.
    const T* ptr() const                            { return _val; }
    T* ptr()                                        { return _val; }

    friend ostream& operator<<(ostream& os, const optional& rhs)    { if (!rhs) return os << "optnull"; return os << *rhs; }

private:
    /// Use generic storage so wrapped object is not constructed until needed
    typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type Storage;

    template<class U>
    void construct(U&& rhs)                         { _val = reinterpret_cast<T*>(&_storage); new (&_storage) T(forward<U>(rhs)); }
    /// Destructs wrapped object if initialized
    void uninit()                                   { if (!_val) return; get().~T(); _val = nullptr; }

    T* _val;
    Storage _storage;
};

/// Specialization for references.
/**
  * The wrapped reference must be bound before it can be assigned, construct with an object ref or call bind().
  * All assignments operate on the bound object.
  * Assigning to optnull will unbind the wrapped reference.
  *
  * \see optional
  */
template<class T>
class optional<T&>
{
    template<class T_> friend class optional;
public:
    optional()                                      : _val(nullptr) {}
    optional(optnull_t)                             : _val(nullptr) {}
    optional(const optional& rhs)                   : _val(rhs._val) {}
    optional(optional& rhs)                         : _val(rhs._val) {}
    optional(optional&& rhs)                        : _val(rhs._val) {}
    template<class U> optional(const optional<U>& rhs)  : _val(rhs._val) {}
    template<class U> optional(optional<U>& rhs)    : _val(rhs._val) {}
    template<class U> optional(optional<U>&& rhs)   : _val(rhs._val) {}
    template<class U> optional(U&& rhs)             : _val(nullptr) { bind(forward<U>(rhs)); }

    /// Bind wrapped reference to object
    template<class U> void bind(U&& rhs)            { _val = &rhs; }
    /// Unbinds object
    optional& operator=(optnull_t)                  { _val = nullptr; return *this; }

    /// Assign wrapped object
    optional& operator=(const optional& rhs)        { return operator=<T&>(rhs); }
    optional& operator=(optional& rhs)              { return operator=<T&>(rhs); }
    optional& operator=(optional&& rhs)             { return operator=<T&>(move(rhs)); }
    template<class U> optional& operator=(const optional<U>& rhs)   { if (rhs._val) operator=(rhs.get()); return *this; }
    template<class U> optional& operator=(optional<U>& rhs)         { if (rhs._val) operator=(rhs.get()); return *this; }
    template<class U> optional& operator=(optional<U>&& rhs)        { if (rhs._val) operator=(move(rhs.get())); return *this; }
    
    /// Assign object.  Reference must be bound to an object before being assigned, this is asserted at runtime.
    template<class U> optional& operator=(U&& rhs)  { get() = forward<U>(rhs); return *this; }
    
    bool operator==(optnull_t) const                { return !_val; }
    bool operator!=(optnull_t) const                { return !operator==(optnull); }

    const T& operator*() const                      { return get(); }
    T& operator*()                                  { return get(); }
    const T* operator->() const                     { return &get(); }
    T* operator->()                                 { return &get(); }
    operator const T&() const                       { return get(); }
    operator T&()                                   { return get(); }

    explicit operator bool() const                  { return _val; }
    explicit operator bool()                        { return _val; }

    const T& get() const                            { assert(_val, "Optional not initialized"); return *_val; }
    T& get()                                        { assert(_val, "Optional not initialized"); return *_val; }

    const T* ptr() const                            { return _val; }
    T* ptr()                                        { return _val; }

    friend ostream& operator<<(ostream& os, const optional& rhs)    { if (!rhs) return os << "optnull"; return os << *rhs; }

private:
    T* _val;
};
    
}
