// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"

/** \cond */
namespace std
{
/** \endcond */
    /// \name iostream methods
    /// @{
    
    /// Allow generic function as manipulator. \ingroup String
    inline ostream& operator<<(ostream& os, function<ostream& (ostream&)>& f)   { return f(os); }
    /// \ingroup String
    inline istream& operator>>(istream& is, function<istream& (istream&)>& f)   { return f(is); }
    /// @}
/** \cond */
}
/** \endcond */

namespace honey
{

/// \addtogroup String
/// @{

/// Base class for iostream manipulators.  Inherit from this class and call `Subclass::inst(ios)` to attach an instance of Subclass to an iostream.
template<class Subclass>
class Manip
{
public:
    static bool hasInst(ios_base& ios)                      { return ios.pword(pword); }
    static Subclass& inst(ios_base& ios)
    {
        if (!hasInst(ios))
        {
            ios.pword(pword) = new Subclass();
            ios.register_callback(&Manip::delete_, 0);
        }
        return *static_cast<Subclass*>(ios.pword(pword));
    }
    
private:
    static void delete_(ios_base::event ev, ios_base& ios, int)
    {
        if (ev != ios_base::erase_event) return;
        honey::delete_(&inst(ios));
    }
    
    static const int pword;
};

template<class Subclass> const int Manip<Subclass>::pword = ios_base::xalloc();

/// \name ostream methods
/// @{

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
inline function<ostream& (ostream&)> indentSize(int size)   { return [=](ostream& os) -> ostream& { priv::Indent::inst(os).size = size; return os; }; }

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



