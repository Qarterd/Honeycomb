// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Object/Property.h"

namespace honey
{

/// \addtogroup Component
/// @{

/// Generic vector property
template<class T>
class Property<vector<T>> : public PropertyBase, public vector<T>
{
public:
    typedef vector<T> List;
    typedef SharedPtr<Property> Ptr;
    typedef SharedPtr<const Property> ConstPtr;

    Property(const String& name)                                    : PropertyBase(name), List(1) {}
    Property(const String& name, const List& list)                  : PropertyBase(name), List(list) {}
    Property(const String& name, List&& list)                       : PropertyBase(name), List(move(list)) {}
    Property(const String& name, szt size, const T& val = T())      : PropertyBase(name), List(size, val) {}
    template<class Iter>
    Property(const String& name, Iter&& first, Iter&& last)         : PropertyBase(name), List(forward<Iter>(first), forward<Iter>(last)) {}
    Property(const Property& rhs)                                   : PropertyBase(rhs._name.name()), List(rhs) {}
    
    static const NameId& s_type();
    virtual const NameId& type() const                              { return s_type(); }
    virtual Property& clone() const                                 { return *new Property(*this); }

    Property& operator=(const Property& rhs)                        { List::operator=(rhs); return *this; }
    Property& operator=(const List& rhs)                            { List::operator=(rhs); return *this; }
    Property& operator=(List&& rhs)                                 { List::operator=(move(rhs)); return *this; }
    /// Assign to first element in list
    Property& operator=(const T& rhs)                               { assert(this->size()); (*this)[0] = rhs; return *this; }
    Property& operator=(T&& rhs)                                    { assert(this->size()); (*this)[0] = move(rhs); return *this; }
    
    /// Retrieve first element
    operator T&()                                                   { return (*this)[0]; }
    operator const T&() const                                       { return (*this)[0]; }
};

/// Integer list property
typedef vector<int> IntList;
template<> inline auto Property<IntList>::s_type() -> const NameId&         { static NameId _("IntList"); return _; }

/// Real list property
typedef vector<Real> RealList;
template<> inline auto Property<RealList>::s_type() -> const NameId&        { static NameId _("RealList"); return _; }

/// String list property
template<> inline auto Property<String::List>::s_type() -> const NameId&    { static NameId _("String::List"); return _; }

/// @}

}
