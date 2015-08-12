// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"

namespace honey
{

/// \addtogroup String
/// @{

/// \name stringstream methods
/// @{

/// Base class to hold iostream manipulator state.  Inherit from this class and call `Subclass::inst(ios)` to attach an instance of Subclass to an iostream.
template<class Subclass>
class Manip
{
public:
    static bool hasInst(ios_base& ios)                                  { return ios.pword(pword); }
    static Subclass& inst(ios_base& ios)
    {
        if (!hasInst(ios)) { ios.pword(pword) = new Subclass(); ios.register_callback(&Manip::delete_, 0); }
        return *static_cast<Subclass*>(ios.pword(pword));
    }
    
private:
    static void delete_(ios_base::event ev, ios_base& ios, int)         { if (ev != ios_base::erase_event) return;honey::delete_(&inst(ios)); }
    static const int pword;
};
template<class Subclass> const int Manip<Subclass>::pword = ios_base::xalloc();

/// \see manipFunc()
template<class Func, class Tuple>
struct ManipFunc
{
    template<class Func_, class Tuple_>
    ManipFunc(Func_&& f, Tuple_&& args)                                 : f(forward<Func_>(f)), args(forward<Tuple_>(args)) {}
    
    friend ostream& operator<<(ostream& os, const ManipFunc& manip)     { manip.apply(os, mt::make_idxseq<tuple_size<Tuple>::value>()); return os; }
    friend istream& operator>>(istream& is, ManipFunc& manip)           { manip.apply(is, mt::make_idxseq<tuple_size<Tuple>::value>()); return is; }
    
    template<size_t... Seq>
    void apply(ostream& os, mt::idxseq<Seq...>) const                   { f(os, get<Seq>(args)...); }
    template<size_t... Seq>
    void apply(istream& is, mt::idxseq<Seq...>) const                   { f(is, get<Seq>(args)...); }
    
    Func f;
    Tuple args;
};

/// Helper to create a manipulator that takes arguments. eg. A manip named 'foo': `auto foo(int val) { return manipFunc([=](ios_base& ios) { FooManip::inst(ios).val = val; }); }`
template<class Func, class... Args>
inline auto manipFunc(Func&& f, Args&&... args)             { return ManipFunc<Func, decltype(make_tuple(forward<Args>(args)...))>(forward<Func>(f), make_tuple(forward<Args>(args)...)); }

/// Shorthand to create ostringstream
inline ostringstream sout()                                 { return ostringstream(); }
    
/** \cond */
namespace priv
{
    struct Indent : Manip<Indent>
    {
        int level = 0;
        int size = 4;
    };
}
extern template class Manip<priv::Indent>;
/** \endcond */

/// Increase stream indent level by 1
inline ostream& indentInc(ostream& os)                      { ++priv::Indent::inst(os).level; return os; }
/// Decrease stream indent level by 1
inline ostream& indentDec(ostream& os)                      { --priv::Indent::inst(os).level; return os; }
/// Set number of spaces per indent level
inline auto indentSize(int size)                            { return manipFunc([=](ostream& os) { priv::Indent::inst(os).size = size; }); }

/// End line and apply any indentation to the next line
inline ostream& endl(ostream& os)
{
    os << std::endl;
    if (priv::Indent::hasInst(os))
    {
        auto& indent = priv::Indent::inst(os);
        for (int i = 0, end = indent.level * indent.size; i < end; ++i) os << ' ';
    }
    return os;
}

/// @}

/// @}

}



