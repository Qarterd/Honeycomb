// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/Misc/StdUtil.h"
#include "Honey/Memory/SharedPtr.h"

namespace honey
{

/// \defgroup stringstream  std::stringstream util
/// @{

/// Shorthand to create ostringstream
inline ostringstream sout()                                 { return ostringstream(); }
    
/// std::stringstream util
namespace stringstream
{
    /** \cond */
    namespace priv
    {
        struct Indent : Manip<Indent>
        {
            int level = 0;
            int size = 4;
        };
    }
    /** \endcond */

    /// Increase stream indent level by 1
    inline ostream& indentInc(ostream& os)                  { ++priv::Indent::inst(os).level; return os; }
    /// Decrease stream indent level by 1
    inline ostream& indentDec(ostream& os)                  { --priv::Indent::inst(os).level; return os; }
    /// Set number of spaces per indent level
    inline auto indentSize(int size)                        { return manipFunc([=](ostream& os) { priv::Indent::inst(os).size = size; }); }
}

/// End line and apply any indentation to the next line
inline ostream& endl(ostream& os)
{
    os << std::endl;
    if (stringstream::priv::Indent::hasInst(os))
    {
        auto& indent = stringstream::priv::Indent::inst(os);
        for (int i = 0, end = indent.level * indent.size; i < end; ++i) os << ' ';
    }
    return os;
}

/// @}

}

/** \cond */
namespace std
{
/** \endcond */
    /// \ingroup stringstream
    /// @{
    
    /// Exception to string
    inline ostream& operator<<(ostream& os, const exception& e)     { return os << e.what(); }
    /// Pair to string
    template<class T1, class T2>
    ostream& operator<<(ostream& os, const pair<T1,T2>& p)  { return os << "[" << p.first << ", " << p.second << "]"; }
    /** \cond */
    namespace priv
    {
        template<class Tuple, size_t... Seq>
        void tupleToString(ostream& os, Tuple&& t, honey::mt::idxseq<Seq...>)
                                                            { os << "["; honey::mt::exec([&]() { os << get<Seq>(forward<Tuple>(t)) << (Seq < sizeof...(Seq)-1 ? ", " : ""); }...); os << "]"; }
        template<class List>
        void listToString(ostream& os, const List& list)    { size_t i = 0; os << "["; for (auto& e: list) os << (i++ > 0 ? ", " : "") << e; os << "]"; }
    }
    /** \endcond */
    /// Tuple to string
    template<class Tuple>
    typename enable_if<honey::mt::isTuple<Tuple>::value, ostream&>::type
        operator<<(ostream& os, Tuple&& t)                  { priv::tupleToString(os, forward<Tuple>(t), honey::mt::make_idxseq<tuple_size<typename honey::mt::removeRef<Tuple>::type>::value>()); return os; }
    /// Array to string
    template<class T, size_t N>
    ostream& operator<<(ostream& os, const array<T,N>& a)   { priv::listToString(os, a); return os; }
    /// C-array to string
    template<class T, size_t N, typename std::enable_if<
                                    !std::is_same<T,char>::value && //stdlib handles char/uint8
                                    !std::is_same<T,honey::uint8>::value &&
                                    !std::is_same<T,honey::Char>::value,int>::type=0> //String handles Char
    ostream& operator<<(ostream& os, const T (&a)[N])       { priv::listToString(os, a); return os; }
    /// Vector to string
    template<class T, class Alloc>
    ostream& operator<<(ostream& os, const vector<T,Alloc>& vec)
                                                            { priv::listToString(os, vec); return os; }
    /// Set to string
    template<class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const set<T,Compare,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Multi-Set to string
    template<class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const multiset<T,Compare,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Unordered Set to string
    template<class Key, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_set<Key,Hash,KeyEqual,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Unordered Multi-Set to string
    template<class Key, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_multiset<Key,Hash,KeyEqual,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Map to string
    template<class Key, class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const map<Key,T,Compare,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Multi-Map to string
    template<class Key, class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const multimap<Key,T,Compare,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Unordered Map to string
    template<class Key, class T, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_map<Key,T,Hash,KeyEqual,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Unordered Multi-Map to string
    template<class Key, class T, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_multimap<Key,T,Hash,KeyEqual,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// UniquePtr to string, outputs object pointed to or 'nullptr'
    template<class T, class Fin>
    ostream& operator<<(ostream& os, const honey::UniquePtr<T,Fin>& p)
                                                            { return p ? os << *p : os << "nullptr"; }
    /// SharedPtr to string, outputs object pointed to or 'nullptr'
    template<class T>
    ostream& operator<<(ostream& os, const honey::SharedPtr<T>& p)
                                                            { return p ? os << *p : os << "nullptr"; }
    /// @}
/** \cond */
}
/** \endcond */

