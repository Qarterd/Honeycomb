// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Object/Component.h"

namespace honey
{

/// \addtogroup Component
/// @{

/// Component object.  Object that consists of a collection of components.
/**
  * A component object contains multiple slots into which components can be inserted.
  * A single slot contains one or many components of the same type.
  */
class ComObject : public Object
{
    friend class Component;

    /// Holds a list of components
    struct Slot
    {
        Slot()                  : type(nullptr) {}
        typedef stdutil::unordered_map<Id, Component*, SmallAllocator> Map;
        typedef vector<Component::Ptr> List;
        Map map;                ///< Look up component by instance id
        List list;              ///< List of components of the same type
        const ComRegistry::Type* type;
    };
    typedef stdutil::unordered_map<Id, Slot, SmallAllocator> SlotMap;

public:
    SIGNAL_DECL(ComObject)
    /// Called after component is inserted
    SIGNAL(sigComInsert, (ComObject& src, Component& com));
    /// Called after component is removed. Reference component with a shared pointer to prevent deletion.
    SIGNAL(sigComRemove, (ComObject& src, Component& com));
    /// Called before instance id is changed
    SIGNAL(sigSetInstId, (ComObject& src, Id id));

    /// Removes and releases all contained components
    virtual ~ComObject()                                        { removeComs(); }

    /// Override from object, sends message about change
    virtual void setInstId(const Id& id)
    {
        listeners().dispatch<sigSetInstId>(*this, id);
        Object::setInstId(id);
    }

    /// Add a component reference. The component is added to the end of its slot (and any supertype slots).
    /**
      * Components contained by this object will be released upon object destruction.
      * \param com          Component to add
      * \param createDeps   Automatically create any missing component dependencies. For performance set to false to avoid walking dep graph.
      *                     Even if false, deps will still be verified in debug mode.
      */ 
    void addCom(Component& com, bool createDeps = false)        { insertCom_priv(com, -1, createDeps); }

    /// Wrapper for pointer arg
    void addCom(Component* com, bool createDeps = false)        { assert(com); addCom(*com, createDeps); }

    /// Add a component reference. The component is inserted into its slot at index.
    /**
      * The component will be also added to the end of any supertype slots.
      * Components contained by this object will be released upon object destruction.
      */ 
    void insertCom(Component& com, int index, bool createDeps = false)
                                                                { assert(index >= 0); insertCom_priv(com, index, createDeps); }

    /// hasComInSlot() with type `Com`
    template<class Com>
    bool hasCom(const Id& id = idnull) const                    { return hasComInSlot(Com::s_comType(), id); }

    /// Check if object contains any components of type (with id).
    bool hasComInSlot(const Id& type, const Id& id = idnull) const
    {
        auto slot = this->slot(type);
        if (!slot) return false;
        if (id != idnull)
        {
            auto it = slot->map.find(id);
            return it != slot->map.end();
        }
        return slot->list.size();
    }

    /// comCountInSlot() with type `Com` 
    template<class Com>
    int comCount() const                                        { return comCountInSlot(Com::s_comType()); }

    /// Get number of components of type that this object contains. O(1) complexity.
    int comCountInSlot(const Id& type) const
    {
        auto slot = this->slot(type);
        return slot ? size(slot->list) : 0;
    }

    /// Get iterator over all slots in this object.  A slot may contain one or many components.
    auto comSlots() const -> decltype(stdutil::keys(declval<const SlotMap>()))
                                                                { return stdutil::keys(_slotMap); } 

    /// Get component of type `Com`. Returns first component in slot, must exist.
    template<class Com>
    Com& com() const
    {
        auto slot = this->slot(Com::s_comType());
        assert(slot && slot->list.size(), sout() << "Component type not found: Request type: " << Com::s_comType());
        return static_cast<Com&>(*slot->list[0]);
    }

    /// comInSlot() with type `Com`
    template<class Com>
    Com& com(const Id& id) const                                { return static_cast<Com&>(comInSlot(Com::s_comType(), id)); }

    /// Get a single component of type with id. If id is null then first component in slot will be returned.
    Component& comInSlot(const Id& type, const Id& id = idnull) const
    {
        Component* com = nullptr;
        auto slot = this->slot(type);
        if (slot)
        {
            if (id != idnull)
            {
                auto it = slot->map.find(id);
                com = it != slot->map.end() ? it->second : nullptr;
            }
            else
                com = slot->list[0];
        }
        assert(com, sout() << "Component type not found: Request type: " << type << " ; Id : " << id);
        return *com;
    }

    /// comsInSlot() with type `Com`
    template<class Com>
    const vector<typename Com::Ptr>& coms() const               { return reinterpret_cast<const vector<typename Com::Ptr>&>(comsInSlot(Com::s_comType())); }

    /// Get all components of type. May return empty list. O(1) complexity.
    const vector<Component::Ptr>& comsInSlot(const Id& type) const
    {
        auto slot = this->slot(type);
        return slot ? slot->list : _nullComs;
    }

    /// Remove a single component. O(n) complexity. 
    /**
      * \param com          Component to remove
      * \param removeDeps   Automatically remove any dependent components. For performance set to false to avoid walking dep graph.
      *                     Even if false, deps will still be verified in debug mode.
      */ 
    void removeCom(Component& com, bool removeDeps = false)
    {
        auto slotIt = _slotMap.find(com.comType());
        if (slotIt == _slotMap.end()) return;
        auto& slot = slotIt->second;
        auto it = std::find(slot.list.begin(), slot.list.end(), &com);
        if (it != slot.list.end()) removeComInSlot(slotIt, it, removeDeps);
    }

    /// Remove a single component of type `Com` with id. O(n) complexity unless id is null. If id is null then first component in slot is removed.
    template<class Com>
    void removeCom(const Id& id = idnull, bool removeDeps = false)
    {
        if (!hasCom<Com>(id)) return;
        removeCom(com<Com>(id), removeDeps);
    }

    /// removeComInSlot() with type `Com`
    template<class Com>
    void removeComAtIndex(int index, bool removeDeps = false)   { removeComInSlot(Com::s_comType(), index, removeDeps); }

    /// Remove component of type at index. O(1) complexity, must exist.
    void removeComInSlot(const Id& type, int index, bool removeDeps = false)
    {
        auto slotIt = _slotMap.find(type);
        assert(slotIt != _slotMap.end() && index >= 0 && index < size(slotIt->second.list));
        removeComInSlot(slotIt, slotIt->second.list.begin() + index, removeDeps);
    }

    /// removeComsInSlot() with type `Com`
    template<class Com>
    void removeComs(bool removeDeps = false)                    { removeComsInSlot(Com::s_comType(), removeDeps); }
    /// Remove all components.  Components are removed in type-dependent order.
    void removeComs();
    /// Remove all components of type.  Components are removed from slot list in reverse order.
    void removeComsInSlot(const Id& type, bool removeDeps = false)
    {
        auto slotIt = _slotMap.find(type);
        if (slotIt == _slotMap.end()) return;
        removeComsInSlot(slotIt, removeDeps);
    }

    /// Get listener list
    ListenerList& listeners()                                   { return _listeners; }

private:
    /// Order slot types by dependencies so that dependents can be removed first
    struct slotDepOrderCmp { bool operator() (const ComRegistry::Type* type, const ComRegistry::Type* type2) const { return type->depOrder() < type2->depOrder(); } };
    typedef multiset<const ComRegistry::Type*, slotDepOrderCmp, SmallAllocator<const ComRegistry::Type*>> SlotDepOrder;
    
    /// Get slot of type
    const Slot* slot(const Id& type) const                      { auto it = _slotMap.find(type); return it != _slotMap.end() ? &it->second : nullptr; }
    Slot* slot(const Id& type)                                  { auto it = _slotMap.find(type); return it != _slotMap.end() ? &it->second : nullptr; }

    /// If index is -1 then add to end
    void insertCom_priv(Component& com, int index, bool createDeps);
    /// `slotIt` will be set to end if slot is erased
    void removeComInSlot(SlotMap::iterator slotIt, Slot::List::iterator it, bool removeDeps);
    void removeComsInSlot(SlotMap::iterator slotIt, bool removeDeps);

    /// Update component id in map.  Call before changing id. 
    void updateComMap(Component& com, const Id& id);

    SlotMap _slotMap;
    SlotDepOrder _slotDepOrder;
    ListenerList _listeners;

    static const vector<Component::Ptr> _nullComs;
};

/// @}

}
