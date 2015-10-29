// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Id.h"
#include "Honey/Math/Real.h"

namespace honey
{

/// \addtogroup Component
/// @{

/// Base class for all properties
class PropertyBase : public Object
{
public:
    typedef SharedPtr<PropertyBase> Ptr;
    typedef SharedPtr<const PropertyBase> ConstPtr;

    PropertyBase(const String& name)                        : _name(name) {}

    /// Get property name
    const String& name() const                              { return _name.name(); }
    /// Get property id
    const Id& id() const                                    { return _name; }
    /// Get property type info
    virtual const NameId& type() const = 0;
    /// Create a clone of this property
    virtual PropertyBase& clone() const = 0;

protected:
    const NameId _name;
};

/// Generic property
template<class T>
class Property : public PropertyBase
{
public:
    typedef SharedPtr<Property> Ptr;
    typedef SharedPtr<const Property> ConstPtr;

    Property(const String& name)                            : PropertyBase(name) {}
    Property(const String& name, const T& val)              : PropertyBase(name), _val(val) {}
    Property(const String& name, T&& val)                   : PropertyBase(name), _val(move(val)) {}
    Property(const Property& rhs)                           : PropertyBase(rhs._name.name()), _val(rhs._val) {}
    
    /// Static function to get property type info
    static const NameId& s_type();
    virtual const NameId& type() const                      { return s_type(); }

    virtual Property& clone() const                         { return *new Property(*this); }

    Property& operator=(const Property& rhs)                { _val = rhs._val; return *this; }
    Property& operator=(const T& rhs)                       { _val = rhs; return *this; }
    Property& operator=(T&& rhs)                            { _val = move(rhs); return *this; }
    
    T& operator*()                                          { return _val; }
    const T& operator*() const                              { return _val; }
    T* operator->()                                         { return &_val; }
    const T* operator->() const                             { return &_val; }
    operator T&()                                           { return _val; }
    operator const T&() const                               { return _val; }
    
private:
    T _val;
};

/// Integer property
template<> inline auto Property<int>::s_type() -> const NameId&         { static NameId _("int"); return _; }
/// Real property
template<> inline auto Property<Real>::s_type() -> const NameId&        { static NameId _("Real"); return _; }
/// String property
template<> inline auto Property<String>::s_type() -> const NameId&      { static NameId _("String"); return _; }

/// @}

}

