// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Object/ComObject.h"

namespace honey
{

const vector<Component::Ptr> ComObject::_nullComs;

void ComObject::insertCom_priv(Component& com, int index, bool createDeps)
{
    static mt::Void _ = ComRegistry::inst().buildDepGraph();  //build dep graph the first time this function is called
    mt_unused(_);
    
    Component::Ptr comPtr = &com;   //prevent destruction in scope
    //remove from previous obj
    if (com.hasObj()) com._comObj->removeCom(com);

    #ifdef DEBUG
        static const bool debug = true;
    #else
        static const bool debug = false;
    #endif
    if (createDeps || debug)
    {
        //add missing component deps
        auto vertex = ComRegistry::inst().depGraph().vertex(com.comType());
        assert(vertex, sout() << "Component not registered: " << com.comType());
        for (auto& e : vertex->links())
        {
            auto& key = *e->keys().begin();
            if (hasComInSlot(key)) continue;
            if (createDeps)
                addCom(ComRegistry::inst().create(key), true);
            else
                error_(sout()   << "Component dependency missing: " << key
                                << ".  Add the missing component first, or add with createDeps = true.");
        } 
    }

    //insert into each type slot in hierarchy
    for (auto& type : com.comType().hierarchy())
    {
        auto& slot = _slotMap[*type];
        if (!slot.type)
        {
            //init slot
            slot.type = type;
            _slotDepOrder.insert(type);
        }
        
        if (type == com.comType().hierarchy().back())
            //main type, insert into place
            slot.list.insert(index != -1 ? slot.list.begin() + index : slot.list.end(), comPtr);
        else
            //supertype, add to end
            slot.list.push_back(comPtr);

        //add instance id to map
        if (com.getInstId() != idnull) slot.map[com.getInstId()] = &com;
    }

    com._comObj = this;
    com.onComInsert();
    listeners().dispatch<sigComInsert>(*this, com);
}

void ComObject::removeComInSlot(SlotMap::iterator slotIt, Slot::List::iterator it, bool removeDeps)
{
    Component::Ptr comPtr = *it;  //prevent destruction in scope
    auto& com = *comPtr;
    const ComRegistry::Type* slotType = slotIt->second.type;

    #ifdef DEBUG
        static const bool debug = true;
    #else
        static const bool debug = false;
    #endif
    if (removeDeps || debug)
    {
        //remove dependent components, check each type slot in hierarchy
        for (auto& type : reversed(com.comType().hierarchy()))
        {
            //only remove deps if this is the last component
            auto slot = type == slotType ? &slotIt->second : this->slot(*type);
            assert(slot);
            if (slot->list.size() > 1) break; //break early, checking supertypes would be redundant
            //search through all slots with greater dep order
            for (auto it = _slotDepOrder.upper_bound(type); it != _slotDepOrder.end();)
            {
                auto& depSlotType = **it++;
                auto vertex = ComRegistry::inst().depGraph().vertex(depSlotType);
                assert(vertex, sout() << "Component not registered: " << depSlotType);
                auto links = vertex->links(ComRegistry::DepNode::DepType::out);
                if (find(links, [&](mt_elemOf(links)& e) { return *e->keys().begin() == *type; }) == links.end()) continue;
                //slot depends on type
                if (removeDeps)
                {
                    auto nextType = it != _slotDepOrder.end() ? *it : nullptr;
                    removeComsInSlot(depSlotType, removeDeps);
                    //if next slot has been erased then choose another
                    if (nextType && !this->slot(*nextType)) it = _slotDepOrder.lower_bound(nextType);
                }
                else
                    assert(!hasComInSlot(depSlotType),
                            sout()  << "Dependent component still in object: " << depSlotType
                                    << ".  Remove the dependent component first, or remove with removeDeps = true.");
            }
        }
    }

    com.onComRemove();
    com._comObj = nullptr;

    //remove from each type slot in hierarchy
    for (auto& type : com.comType().hierarchy())
    {
        auto slotIt_ = type == slotType ? slotIt : _slotMap.find(*type);
        assert(slotIt_ != _slotMap.end());
        auto& slot = slotIt_->second;

        //erase from list
        if (type == slotType)
            slot.list.erase(it);
        else
        {
            //erase from type in hierarchy, search for com starting at back
            auto range = reversed(slot.list);
            auto it = std::find(range.begin(), range.end(), comPtr);
            if (it != range.end()) stdutil::erase(slot.list, it);
        }

        //erase instance id from map
        if (com.getInstId() != idnull)
        {
            auto itMap = slot.map.find(com.getInstId());
            if (itMap != slot.map.end() && itMap->second == &com)
                slot.map.erase(itMap);
        }

        //erase slot if it doesn't hold any more components
        if (slot.list.empty())
        {
            _slotMap.erase(slotIt_);
            _slotDepOrder.erase(stdutil::findVal(_slotDepOrder, type));
        }
    }

    listeners().dispatch<sigComRemove>(*this, com);
}

void ComObject::removeComsInSlot(SlotMap::iterator slotIt, bool removeDeps)
{
    //remove coms in reverse order, check if slot has been erased on each iteration
    auto& type = *slotIt->second.type;
    while (slot(type)) removeComInSlot(slotIt, --slotIt->second.list.end(), removeDeps);
}

void ComObject::removeComs()
{
    //remove slots in reverse dep order
    while (!_slotDepOrder.empty()) removeComsInSlot(**--_slotDepOrder.end(), false);
}

void ComObject::updateComMap(Component& com, const Id& id)
{
    //update each type slot in hierarchy
    for (auto& type : com.comType().hierarchy())
    {
        auto slot = this->slot(*type);
        assert(slot);
        auto it = slot->map.find(com.getInstId());
        if (it != slot->map.end() && it->second == &com) slot->map.erase(it);
        slot->map[id] = &com;
    }
}

}