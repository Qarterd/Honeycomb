// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Numeral.h"
#include "Honey/Misc/Enum.h"
#include "Honey/Misc/Range.h"

namespace honey
{

/// Methods that extend the functionality of the standard library.
/**
  * \defgroup StdUtil   Standard Util
  */
/// @{

/// Safely get the size of a std container as a signed integer. The size() member method returns size_t which results in a conversion warning.
template<class StdContainer>
int size(const StdContainer& cont)                      { return numeric_cast<int>(cont.size()); }

/// Create a range over the keys of a map or map iterator range. \see values()
template<class Range>
auto keys(Range&& range) -> Range_<TupleIter<mt_iterOf(range),0>, TupleIter<mt_iterOf(range),0>>
{
    return honey::range(TupleIter<mt_iterOf(range),0>(begin(range)), TupleIter<mt_iterOf(range),0>(end(range)));
}

/// Create a range over the values of a map or map iterator range. \see keys()
template<class Range>
auto values(Range&& range) -> Range_<TupleIter<mt_iterOf(range),1>, TupleIter<mt_iterOf(range),1>>
{
    return honey::range(TupleIter<mt_iterOf(range),1>(begin(range)), TupleIter<mt_iterOf(range),1>(end(range)));
}

/// See \ref StdUtil
namespace stdutil
{
    /// Convert reverse iterator to forward iterator
    template<class Iter>
    auto reverseIterToForward(Iter&& it) -> typename mt::removeRef<decltype(--it.base())>::type
                                                        { return --it.base(); }

    /// Erase using reverse iterator.  Returns reverse iterator to element after erased element.
    template<class List>
    typename List::reverse_iterator erase(List& list, const typename List::reverse_iterator& iter)
    {
        return typename List::reverse_iterator(list.erase(reverseIterToForward(iter)));
    }

    /// Erase first occurrence of value.  Returns iterator to next element after the erased element, or end if not found.
    template<class List>
    typename List::iterator eraseVal(List& list, const typename List::value_type& val)
    {
        auto it = std::find(list.begin(), list.end(), val);
        if (it == list.end()) return list.end();
        return list.erase(it);
    }

    /// Erase all occurrences of value.
    template<class List, class T>
    void eraseVals(List& list, const T& val)
    {
        auto it = list.begin();
        while((it = std::find(it, list.end(), val)) != list.end())
            it = list.erase(it);
    }

    /// Get iterator to key with value.  Returns end if not found.
    template<class MultiMap, class Key, class Val>
    auto findVal(MultiMap& map, const Key& key, const Val& val) -> mt_iterOf(map)
    {
        return honey::find(range(map.equal_range(key)), [&](auto& e) { return e.second == val; });
    }
    /// Get iterator to value.  Returns end if not found.
    template<class MultiSet, class Val>
    auto findVal(MultiSet& set, const Val& val) -> mt_iterOf(set)
    {
        return honey::find(range(set.equal_range(val)), [&](auto& e) { return e == val; });
    }

    /// std::unordered_map with custom allocator
    template<class Key, class Value, template<class> class Alloc>
    using unordered_map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, Alloc<pair<const Key, Value>>>;
    /// std::unordered_multimap with custom allocator
    template<class Key, class Value, template<class> class Alloc>
    using unordered_multimap = std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>, Alloc<pair<const Key, Value>>>;
    /// std::unordered_set with custom allocator
    template<class Key, template<class> class Alloc>
    using unordered_set = std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, Alloc<Key>>;
    /// std::unordered_multiset with custom allocator
    template<class Key, template<class> class Alloc>
    using unordered_multiset = std::unordered_multiset<Key, std::hash<Key>, std::equal_to<Key>, Alloc<Key>>;
}

/** \cond */
namespace priv
{
    template<int Arity> struct bind_fill;

    #define PARAMT(It)          , class T##It
    #define PARAM(It)           , T##It&& a##It
    #define ARG(It)             , forward<T##It>(a##It)
    #define PLACE(It)           , _##It

    #define OP(It, ItMax)                                                                               \
        template<class Func ITERATE__(1,It,PARAMT)>                                                     \
        auto operator()(Func&& f ITERATE__(1,It,PARAM)) ->                                              \
            decltype(   bind(forward<Func>(f) ITERATE__(1,It,ARG)                                       \
                            IFEQUAL(It,ItMax,,ITERATE__(1,PP_SUB(ItMax,It),PLACE))) )                   \
        {                                                                                               \
            return      bind(forward<Func>(f) ITERATE__(1,It,ARG)                                       \
                            IFEQUAL(It,ItMax,,ITERATE__(1,PP_SUB(ItMax,It),PLACE)));                    \
        }                                                                                               \
    
    #define STRUCT(It)                                                                                  \
        template<> struct bind_fill<It>     { ITERATE1_(0, It, OP, It) };                               \

    ITERATE(0, 9, STRUCT)
    #undef PARAMT
    #undef PARAM
    #undef ARG
    #undef PLACE
    #undef OP
    #undef STRUCT
}
/** \endcond */

/// Version of bind that automatically fills in placeholders for unspecified arguments.
/**
  * ### Example
  * To bind the `this` pointer for a member function: `void Class::func(int, int)`
  * \hiddentable
  * \row The standard bind method is:               \col `bind(&Class::func, this, _1, _2);` \endrow
  * \row `bind_fill` has a more convenient syntax:  \col `bind_fill(&Class::func, this);`    \endrow
  * \endtable
  */
template<class Func, class... Args>
auto bind_fill(Func&& f, Args&&... args) ->
    decltype(   priv::bind_fill<mt::funcTraits<typename mt::removeRef<Func>::type>::arity>()(forward<Func>(f), forward<Args>(args)...))
{
    return      priv::bind_fill<mt::funcTraits<typename mt::removeRef<Func>::type>::arity>()(forward<Func>(f), forward<Args>(args)...);
}

/// Wraps a pointer so that it behaves similar to a reference.
/**
  * In a std container, this wrapper type can be used instead of an object pointer so that the constness of the container carries through to its objects.
  */
template<class T, class Ptr = T*, class ConstPtr = const T*>
class deref_wrap
{
public:
    deref_wrap(Ptr ptr = nullptr)                   : _ptr(ptr) {}
    T& operator*()                                  { assert(_ptr); return *_ptr; }
    const T& operator*() const                      { assert(_ptr); return *_ptr; }
    T* operator->()                                 { assert(_ptr); return _ptr; }
    const T* operator->() const                     { assert(_ptr); return _ptr; }
    operator T&()                                   { assert(_ptr); return *_ptr; }
    operator const T&() const                       { assert(_ptr); return *_ptr; }
    bool operator==(const deref_wrap& rhs) const    { return _ptr == rhs._ptr; }
    bool operator!=(const deref_wrap& rhs) const    { return !operator==(rhs); }
    Ptr& ptr()                                      { return _ptr; }
    const ConstPtr& ptr() const                     { return reinterpret_cast<const ConstPtr&>(_ptr); }
private:
    Ptr _ptr;
};

/// Allows for recursive type definitions, eg. `class Object : vector<recursive_wrap<Object>>`.
/**
  * This wrapper dynamically allocates the object and stores a pointer internally.
  * The wrapper interface provides value semantics to the dynamic object.
  */
template<class T, class Alloc = typename DefaultAllocator<T>::type>
class recursive_wrap
{
public:
    recursive_wrap(Alloc a = Alloc())                       : _ptr(new (a.allocate(1)) T, move(a)) {}
    template<class T_>
    recursive_wrap(T_&& t, Alloc a = Alloc())               : _ptr(new (a.allocate(1)) T(forward<T_>(t)), move(a)) {}
    recursive_wrap(const recursive_wrap& rhs)               : _ptr(new (const_cast<Alloc&>(rhs._ptr.finalizer().a).allocate(1)) T(*rhs), rhs._ptr.finalizer()) {}
    recursive_wrap(recursive_wrap& rhs)                     : _ptr(new (rhs._ptr.finalizer().a.allocate(1)) T(*rhs), rhs._ptr.finalizer()) {}
    recursive_wrap(recursive_wrap&& rhs)                    : _ptr(move(rhs._ptr)) {}
    
    template<class T_>
    recursive_wrap& operator=(T_&& t)                       { **this = forward<T_>(t); return *this; }
    recursive_wrap& operator=(const recursive_wrap& rhs)    { **this = *rhs; return *this; }
    recursive_wrap& operator=(recursive_wrap& rhs)          { **this = *rhs; return *this; }
    recursive_wrap& operator=(recursive_wrap&& rhs)         { _ptr = move(rhs._ptr); return *this; }
    
    /// Get the wrapped object
    T& operator*()                                          { assert(_ptr); return *_ptr; }
    const T& operator*() const                              { assert(_ptr); return *_ptr; }
    T* operator->()                                         { assert(_ptr); return _ptr; }
    const T* operator->() const                             { assert(_ptr); return _ptr; }
    operator T&()                                           { assert(_ptr); return *_ptr; }
    operator const T&() const                               { assert(_ptr); return *_ptr; }
    /// Get the internal pointer, may be null if wrapper was moved.
    T* ptr()                                                { return _ptr; }
    const T* ptr() const                                    { return _ptr; }
    
private:
    UniquePtr<T, finalize<T, Alloc>> _ptr;
};

/// @}

}

