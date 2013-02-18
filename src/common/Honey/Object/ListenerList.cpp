// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Object/ListenerList.h"

namespace honey
{

void ListenerList::add(const Listener& listener)
{
    SpinLock::Scoped _(_lock);

    //Add listener to object map
    _objMap.insert(make_pair(listener.obj(), &listener));

    //Add listener slot to signal map
    auto& slot = listener.slot();
    auto& index = _signalMap[slot.signalId()];
    index.list.push_back(&slot);
    index.map.insert(make_pair(&slot, --index.list.end()));

    if (_cb) _cb->onAdd(listener);
}

void ListenerList::remove(const Listener& listener)
{
    SpinLock::Scoped _(_lock);
    Listener::ConstPtr __ = &listener;  //Prevent destruction in scope

    //Remove listener from object map
    auto itMap = stdutil::findVal(_objMap, listener.obj(), &listener);
    if (itMap != _objMap.end()) _objMap.erase(itMap);

    //Remove listener slot from signal map
    do
    {
        auto& slot = listener.slot();
        auto itMap = _signalMap.find(slot.signalId());
        if (itMap == _signalMap.end()) break;
        auto& index = itMap->second;
        auto itIndex = index.map.find(&slot);
        if (itIndex == index.map.end()) break;
        index.list.erase(itIndex->second);
        index.map.erase(itIndex);
        if (index.list.empty()) _signalMap.erase(itMap);
    } while (false);

    if (_cb) _cb->onRemove(listener);
}

void ListenerList::remove(const void* obj)
{
    SpinLock::Scoped _(_lock);
    auto itPair = _objMap.equal_range(obj);
    for (auto it = itPair.first; it != itPair.second;) remove(*it++->second);
}

void ListenerList::remove(const void* obj, const Id& id)
{
    SpinLock::Scoped _(_lock);
    auto itPair = _objMap.equal_range(obj);
    for (auto it = itPair.first; it != itPair.second;)
    {
        auto& listener = it++->second;
        if (listener->id() != id) continue;
        remove(*listener);
    }
}

void ListenerList::clear()
{
    SpinLock::Scoped _(_lock);
    while (!_objMap.empty()) remove(*_objMap.begin()->second);
}

const ListenerList::SlotList* ListenerList::slotList(const Id& signalId) const
{
    auto itMap = _signalMap.find(signalId);
    if (itMap == _signalMap.end()) return nullptr;
    return &itMap->second.list;
}

}
