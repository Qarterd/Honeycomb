// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Object/Component.h"
#include "Honey/Object/ComObject.h"

namespace honey
{

mt::Void ComRegistry::buildDepGraph()
{
    //Build dep graph
    _depGraph.clear();
    for (auto& e : _types)
    {
        //Create type dep node
        Type& type = *e.second;
        type._depNode = type._depCreate();
        type._depNode.setData(&type);
        type._depNode.setKey(type);
        _depGraph.add(type._depNode);
    }

    //Calculate dep order using a specialized dep graph
    //For dep order, we need to expand any specified links to also link the entire subtype tree
    DepGraph graph;
    list<DepNode> nodes;
    for (auto& e : _types)
    {
        Type& type = *e.second;
        type._depOrder = -1;
        nodes.push_back(type._depNode);
        auto& node = nodes.back();
        //iterate over copy of deps, adding subtype trees
        auto deps = node.deps(); 
        node.clear();
        for (auto& e : deps)
        {
            Type& other = this->type(e.first);
            auto depType = e.second;
            for (auto& e : other._node.preOrd()) node.add(**e, depType);
        }
        graph.add(node);
    }

    //For each node, move along in-edge subgraph and increase visit count.
    //Note that this is O(n^2*v) where n is in-edge depth and v is vertex count.
    auto inner = graph.range();  //Re-use inner iter for performance
    for (auto& e : graph.range())
    {
        inner.begin().reset(*e.keys().begin(), DepNode::DepType::in);
        for (auto& it = inner.begin(); it != inner.end(); ++it)
            ++(**it->nodes().begin())->_depOrder;
    }
    return mt::Void();
}

void Component::setInstId(const Id& id)
{
    if (_comObj) _comObj->updateComMap(*this, id);
    Object::setInstId(id);
}

}