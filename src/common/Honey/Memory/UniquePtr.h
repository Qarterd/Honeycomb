// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Misc/Debug.h"

namespace honey
{

/// \addtogroup Memory
/// @{

/// Pointer to a unique, non-shared, object.  Finalizer is run upon destruction (deletes object by default) if pointer is not null.
template<class T, class Fin = finalize<T>>
class UniquePtr : mt::NoCopy
{
    template<class, class> friend class UniquePtr;
    
    template<class T_> struct Elem_                                 { typedef T_ type; };
    template<class T_> struct Elem_<T_[]>                           { typedef T_ type; };
    template<class T_, bool = std::is_void<T_>::value> struct Ref_  { typedef T_& type; static type deref(T_* p) { return *p; } };
    template<class T_> struct Ref_<T_, true>                        { typedef T_ type;  static type deref(T_* p) {} };
    
public:
    typedef typename Elem_<T>::type Elem;
    typedef typename Ref_<Elem>::type Ref;
    typedef Elem* Ptr;
    
    UniquePtr()                                                     : _ptr(nullptr) {}
    template<class Fin_ = Fin>
    UniquePtr(Ptr ptr, Fin_&& f = Fin_())                           : _ptr(ptr), _fin(forward<Fin_>(f)) {}
    /// Moves pointer and finalizer out of rhs.  To set a new finalizer into rhs use move assign: rhs = UniquePtr(p,f);
    UniquePtr(UniquePtr&& rhs) noexcept                             : _ptr(rhs.release()), _fin(forward<Fin>(rhs._fin)) {}
    template<class U, class F> UniquePtr(UniquePtr<U,F>&& rhs)      : _ptr(rhs.release()), _fin(forward<F>(rhs._fin)) {}

    ~UniquePtr()                                                    { if (_ptr) _fin(_ptr); }

    /// Set ptr
    UniquePtr& operator=(Ptr rhs)                                   { set(rhs); return *this; }

    /// Moves pointer and finalizer out of rhs
    UniquePtr& operator=(UniquePtr&& rhs)                           { return operator=<T,Fin>(move(rhs)); }
    template<class U, class F>
    UniquePtr& operator=(UniquePtr<U,F>&& rhs)                      { set(rhs.release()); _fin = forward<F>(rhs._fin); return *this; }

    template<class U, class F>
    bool operator==(const UniquePtr<U,F>& rhs) const                { return get() == rhs.get(); }
    template<class U, class F>
    bool operator!=(const UniquePtr<U,F>& rhs) const                { return !operator==(rhs); }
    template<class U, class F>
    bool operator< (const UniquePtr<U,F>& rhs) const                { return std::less<typename std::common_type<Ptr, typename UniquePtr<U,F>::Ptr>::type>()(get(), rhs.get()); }
    template<class U, class F>
    bool operator> (const UniquePtr<U,F>& rhs) const                { return rhs.operator<(*this); }
    template<class U, class F>
    bool operator<=(const UniquePtr<U,F>& rhs) const                { return !operator>(rhs); }
    template<class U, class F>
    bool operator>=(const UniquePtr<U,F>& rhs) const                { return !operator<(rhs); }
    
    Ptr operator->() const                                          { assert(_ptr); return _ptr; }
    Ref operator*() const                                           { assert(_ptr); return Ref_<Elem>::deref(_ptr); }
    operator Ptr() const                                            { return _ptr; }

    /// Get the raw pointer to the object
    Ptr get() const                                                 { return _ptr; }

    /// Get the finalizer
    Fin& finalizer()                                                { return _fin; }
    const Fin& finalizer() const                                    { return _fin; }

    /// Give up ownership of pointer without finalizing and set to null
    Ptr release()                                                   { Ptr tmp = _ptr; _ptr = nullptr; return tmp; }

    /// Finalize old pointer and assign new.  Does not finalize if old pointer is the same or null.
    void set(Ptr p)
    {
        if (_ptr != p)
        {
            Ptr old = _ptr;
            _ptr = p;
            if (old) _fin(old);
        }
    }

private:
    Ptr _ptr;
    Fin _fin;
};

/// Create a unique ptr to an object of type T constructed with args
/** \relates UniquePtr */
template<class T, class Alloc, class... Args, class Fin = finalize<T,typename mt::removeRef<Alloc>::type>>
UniquePtr<T,Fin> alloc_unique(Alloc&& a, Args&&... args)            { return UniquePtr<T,Fin>(new (a.allocate(1)) T(forward<Args>(args)...), Fin(forward<Alloc>(a))); }
/// alloc_unique() using T::Allocator if available, otherwise std::allocator  
/** \relates UniquePtr */
template<class T, class... Args, typename mt::disable_if<std::is_array<T>::value, int>::type=0, class Alloc = typename DefaultAllocator<T>::type>
UniquePtr<T> make_unique(Args&&... args)                            { return alloc_unique<T>(Alloc(), forward<Args>(args)...); }
/// Create a unique ptr to an array of `size` number of elements
/** \relates UniquePtr */
template<class T, class... Args, typename std::enable_if<std::is_array<T>::value, int>::type=0>
UniquePtr<T> make_unique(szt size)                                  { return UniquePtr<T>(new typename UniquePtr<T>::Elem[size]()); }
/// Create a unique ptr to an array with deduced size
/** \relates UniquePtr */
template<class T, class... Args, typename std::enable_if<std::is_array<T>::value, int>::type=0>
UniquePtr<T> make_unique_auto_size(Args&&... args)                  { return UniquePtr<T>(new typename UniquePtr<T>::Elem[sizeof...(Args)]{forward<Args>(args)...}); }

/// @}

}

/** \cond */
namespace std
{
    /// Allow class to be used as key in unordered containers
    template<class T, class Fin>
    struct hash<honey::UniquePtr<T,Fin>>
    {
        size_t operator()(const honey::UniquePtr<T,Fin>& val) const     { return reinterpret_cast<size_t>(val.get()); };
    };
}
/** \endcond */