// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Object/Object.h"
#include "Honey/Object/Property.h"
#include "Honey/Memory/SmallAllocator.h"

namespace honey
{

/// \addtogroup Component
/// @{
struct PropertyError : Exception                                        { EXCEPTION(PropertyError) };

/** \cond */
namespace property { namespace priv
{
    /// Called by PropertyObject to create a property for type T. May be specialized for a type.
    template<class T> static Property<T>& create(const String& name)    { return *new Property<T>(name); }
} }
/** \endcond */

/// Object that contains properties
class PropertyObject : public Object
{
public:
    typedef stdutil::unordered_map<Id, PropertyBase::Ptr, SmallAllocator> PropertyMap;
    typedef stdutil::unordered_map<Id, PropertyBase::ConstPtr, SmallAllocator> PropertyMapConst;
    
    PropertyObject() {}

    /// Releases all contained properties
    virtual ~PropertyObject() {}

    /// Add a property reference. Any existing property with the same id will be released and replaced.
    /**
      * Properties contained by this object will be released upon object destruction.
      */ 
    void addProp(PropertyBase& prop)
    {
        assert(prop.id() != idnull, "Property must have valid id");
        PropertyBase::Ptr ptr = &prop;
        auto res = _propMap.insert(make_pair(prop.id(), ptr));
        if (res.second) return;
        res.first->second = ptr;
    }

    /// Wrapper for pointer arg
    void addProp(PropertyBase* prop)                                    { assert(prop); addProp(*prop); }

    /// Check if object contains property with id
    bool hasProp(const Id& id) const                                    { return _propMap.find(id) != _propMap.end(); }

    /// Check if object contains property with id and type T
    template<class T>
    bool hasProp(const Id& id) const
    {
        auto it = _propMap.find(id);
        return it != _propMap.end() && it->second->type() == Property<T>::s_type();
    }

    /// Get property with name of type T.  Adds property if it doesn't exist.
    template<class T>
    Property<T>& prop(const String& name)
    {
        Id id = name;
        PropertyBase* prop;
        auto it = _propMap.find(id);
        if (it != _propMap.end())
            prop = it->second;
        else
        {
            //Property doesn't exist, add it
            prop = &property::priv::create<T>(name);
            addProp(*prop);
        }
        assert(prop->type() == Property<T>::s_type(),
                    sout()  << "Component type mismatch: "
                            << "Request: " << Property<T>::s_type() << " ; Id: " << id
                            << " ; Found: " << prop->type());
        return static_cast<Property<T>&>(*prop);
    }
    
    /// Get property with id of type T.  Throws PropertyError if property doesn't exist.
    template<class T>
    Property<T>& prop(const Id& id)                                     { return prop_<T>(id); }
    template<class T>
    const Property<T>& prop(const Id& id) const                         { return prop_<T>(id); }
    
    /// Get property with id.  Throws PropertyError if property doesn't exist.
    PropertyBase& prop(const Id& id)                                    { return prop_(id); }
    const PropertyBase& prop(const Id& id) const                        { return prop_(id); }
    
    /// Assign property with name to value.  Adds property if it doesn't exist.  Returns assigned property.
    template<class T, class T_ = typename mt::removeRef<T>::type>
    Property<T_>& prop(const String& name, T&& val)                     { return prop<T_>(name) = forward<T>(val); }
    /// Assign property with id to value.  Throws PropertyError if property doesn't exist.  Returns assigned property.
    template<class T, class T_ = typename mt::removeRef<T>::type>
    Property<T_>& prop(const Id& id, T&& val)                           { return prop<T_>(id) = forward<T>(val); }
    
    /// Get all properties
    const PropertyMap& props()                                          { return _propMap; }
    const PropertyMapConst& props() const                               { return reinterpret_cast<const PropertyMapConst&>(_propMap); }
        
    /// Remove a single property. Returns property if found and removed.
    PropertyBase::Ptr removeProp(PropertyBase& prop)                    { return removeProp(prop.id()); }

    /// Remove a single property with id. Returns property if found and removed.
    PropertyBase::Ptr removeProp(const Id& id)
    {
        auto it = _propMap.find(id);
        return it != _propMap.end() ? removeProp(it) : nullptr;
    }

    /// Remove all properties. Calls functor for each removed property.
    void removeProps(const function<void (PropertyBase&)>& f = [](PropertyBase&){})
    {
        while (!_propMap.empty()) { f(*removeProp(_propMap.begin())); }
    }

private:
    template<class T>
    Property<T>& prop_(const Id& id) const
    {
        auto it = _propMap.find(id);
        if (it == _propMap.end()) throw_ PropertyError() << (sout() << "Property not found. Id: " debug_if(<< id));
        PropertyBase* prop = it->second;
        assert(prop->type() == Property<T>::s_type(),
                    sout()  << "Component type mismatch: "
                            << "Request: " << Property<T>::s_type() << " ; Id: " << id
                            << " ; Found: " << prop->type());
        return static_cast<Property<T>&>(*prop);
    }
    
    PropertyBase& prop_(const Id& id) const
    {
        auto it = _propMap.find(id);
        if (it == _propMap.end()) throw_ PropertyError() << (sout() << "Property not found. Id: " debug_if(<< id));
        return *it->second;
    }
    
    /// Remove a property
    PropertyBase::Ptr removeProp(PropertyMap::iterator it)
    {
        PropertyBase::Ptr prop = it->second;
        _propMap.erase(it);
        return prop;
    }

    PropertyMap _propMap;
};

/// @}

}

