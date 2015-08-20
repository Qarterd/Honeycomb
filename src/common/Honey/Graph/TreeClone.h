// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Graph/Tree.h"
#include "Honey/Object/ListenerQueue.h"

namespace honey
{

/// Clone and track changes to an entire tree
template<class TreeNode>
class TreeClone
{
public:

    TreeClone()
    {
        _listeners.push_back(&ListenerQueue::create<typename TreeNode::sigDestroy>(bind_fill(&TreeClone::onDestroy, this)));
        _listeners.push_back(&ListenerQueue::create<typename TreeNode::sigSetData>(bind_fill(&TreeClone::onSetData, this)));
        _listeners.push_back(&ListenerQueue::create<typename TreeNode::sigSetKey>(bind_fill(&TreeClone::onSetKey, this)));
        _listeners.push_back(&ListenerQueue::create<typename TreeNode::sigInsertChild>(bind_fill(&TreeClone::onInsertChild, this)));
        _listeners.push_back(&ListenerQueue::create<typename TreeNode::sigRemoveChild>(bind_fill(&TreeClone::onRemoveChild, this)));
    }

    ~TreeClone()                                        { clear(); }

    /// Register a node to clone. Returns clone. Clone's state is invalid (not equal to original) until update() is called.
    /**
      * Cloned nodes will track changes to the data and hierarchy of the original node.
      * Any subtrees formed under the original node are automatically tracked and mirrored in the clone.
      *
      * While registered, the only type of clone safe to be tampered with is a "root clone".
      * A root clone is a clone whose parent is not registered.  A root clone is special because it can be attached as
      * a child to any other tree.  Do not perform any other operations to registered clones.
      *
      * Warning:    if an original node is destroyed while still registered, then its clone will also be destroyed
      *             on the next update() call.
      */
    TreeNode& regNode(const TreeNode& rootNode)
    {
        //Check if node is already registered
        auto clone = getRegClone(rootNode);
        if (clone) return *clone;
        //Create a phantom that will be made into a proper clone after update
        return createPhantom(rootNode);
    }

    /// Stop tracking changes to a node and its entire subtree.  Returns clone of first node unregistered (or null if none found).
    /**
      * This function will fail if rootNode's parent is registered (branches of registered trees can't be detached).
      *
      * Clone and its subtree are free to be used like normal nodes.
      * Resources for clone and its subtree are held by this class.
      *
      * Warning:    If the original node is re-registered or attached to a registered tree then it will automatically use
      *             the same clone resource again.  Clones in use are not to be tampered with (check isRegClone).
      */
    TreeNode* unregNode(const TreeNode& rootNode)
    {
        TreeNode* ret = nullptr;

        //We must be completely up-to-date to safely unregister
        update();

        for (auto& e : rootNode.preOrd())
        {
            auto clone = unregNodeSingle(e);
            if (!ret) ret = clone;
        }

        return ret;
    }

    /// Stop a clone and its entire subtree from tracking changes to their original nodes.  Returns original node of first clone unregistered (or null if none found).
    /**
      * This function will fail if rootClone's parent is registered.
      * \see unregNode()
      */
    const TreeNode* unregClone(const TreeNode& rootClone)
    {
        const TreeNode* ret = nullptr;

        //We must be completely up-to-date to safely unregister
        update();

        for (auto& e : rootClone.preOrd())
        {
            auto node = getOrigNode(e);
            if (!node) continue;
            auto res = unregNodeSingle(*node);
            if (!ret && res) ret = node;
        }

        return ret;
    }

    /// Update clones to make them mirror the current data and hierarchy of registered nodes
    void update()
    {
        //Process all signals to bring cloned nodes into the latest up-to-date state
        for (auto& e : _listeners) e->process();

        //Clone any phantoms that were registered or attached as unknown children
        while (!_phantomMap.empty())
            cloneTree(*_phantomMap.begin()->first);
    }

    /// Reset the state of the tree clone structure, unregister all nodes and destroy all clones 
    void clear()
    {
        //Remove listeners from orig nodes
        for (auto& node : keys(_regMap)) for (auto& e : _listeners) const_cast<TreeNode*>(node)->listeners().remove(*e);
        for (auto& e : _listeners) e->clear();

        //Free clone resources
        deleteRange(values(_cloneMap));
        _cloneMap.clear();
        _cloneRMap.clear();
        _regMap.clear();
        _phantomMap.clear();
        _unregMap.clear();
    }

    /// Get the cloned version of a node (node may be registered or unregistered).  Returns null if not found.
    TreeNode* getClone(const TreeNode& node) const
    {
        auto it = _cloneMap.find(&node);
        if (it == _cloneMap.end()) return nullptr;
        return it->second;
    }

    /// Get the original node of a clone.  Returns null if not found.
    const TreeNode* getOrigNode(const TreeNode& clone) const
    {
        auto it = _cloneRMap.find(const_cast<TreeNode*>(&clone));
        if (it == _cloneRMap.end()) return nullptr;
        return it->second;
    }

    /// Check if a node is registered
    bool isRegNode(const TreeNode& node) const          { return getRegClone(node); }
    /// Check if a clone is registered
    bool isRegClone(const TreeNode& clone) const        { auto node = getOrigNode(clone); return node ? isRegNode(*node) : false; }

    /// Get total number of clones being handled by this tree
    szt cloneCount() const                              { return _cloneMap.size(); }
    /// Get total number of registered nodes
    szt regNodeCount() const                            { return _regMap.size() + _phantomMap.size(); }

private:

    void onDestroy(TreeNode& src)
    {
        auto clone = getRegClone(src);
        assert(clone && !_phantomMap.count(&src) && !_unregMap.count(&src));
        deleteClone(*clone);
    }

    void onSetData(TreeNode& src, const typename TreeNode::Data& data)
    {
        auto clone = getRegClone(src);
        assert(clone && !_phantomMap.count(&src) && !_unregMap.count(&src));
        clone->setData(data);
    }

    void onSetKey(TreeNode& src, const typename TreeNode::Key& key)
    {
        auto clone = getRegClone(src);
        assert(clone && !_phantomMap.count(&src) && !_unregMap.count(&src));
        clone->setKey(key);
    }

    void onInsertChild(TreeNode& src, TreeNode* childPos, TreeNode& child)
    {
        auto clone = getRegClone(src);
        assert(clone && !_phantomMap.count(&src) && !_unregMap.count(&src));
        
        //A null original child means insert at end of list
        auto it = end(clone->children());
        if (childPos)
        {
            //Original child not null, get the original child's clone
            childPos = getRegClone(*childPos);
            assert(childPos && !_unregMap.count(childPos));
            it = clone->childPos(*childPos);
        }

        //Child could be an unknown node, create phantom if necessary
        auto& cloneChild = regNode(child);

        clone->insertChild(it, cloneChild);
    }

    void onRemoveChild(TreeNode& src, TreeNode& child)
    {
        auto clone = getRegClone(src);
        assert(clone && !_phantomMap.count(&src) && !_unregMap.count(&src));

        //Get the original child's clone
        auto cloneChild = getRegClone(child);
        assert(cloneChild && !_unregMap.count(cloneChild));

        auto it = clone->childPos(*cloneChild);
        assert(it != end(clone->children()));

        clone->removeChild(it);
    }

    void deleteClone(TreeNode& clone)
    {
        auto it = _cloneRMap.find(&clone);
        assert(it != _cloneRMap.end() && it->second && !_phantomMap.count(it->second) && !_unregMap.count(it->second));
        _cloneMap.erase(it->second);
        _regMap.erase(it->second);
        _cloneRMap.erase(it);
        delete_(&clone);
    }

    TreeNode& allocClone(const TreeNode& node)
    {
        assert(!_cloneMap.count(&node));
        auto clone = new TreeNode();
        _cloneMap[&node] = clone;
        _cloneRMap[clone] = &node;
        return *clone;
    }

    TreeNode* phantom(const TreeNode& node) const
    {
        auto it = _phantomMap.find(&node);
        if (it == _phantomMap.end()) return nullptr;
        return it->second;
    }

    TreeNode* getRegClone(const TreeNode& regNode) const
    {
        auto it = _regMap.find(&regNode);
        if (it == _regMap.end()) return nullptr;
        return it->second;
    }

    TreeNode* getUnregClone(const TreeNode& node) const
    {
        auto it = _unregMap.find(&node);
        if (it == _unregMap.end()) return nullptr;
        return it->second;
    }

    /// Creates a cloned node from an unknown node.  Phantom is not yet up-to-date and not trackable.
    TreeNode& createPhantom(const TreeNode& node)
    {
        //Phantom must not already exist
        assert(!_phantomMap.count(&node));
        //Pull a new clone from the unregistered list if possible
        auto phantom = getUnregClone(node);
        if (phantom) _unregMap.erase(&node);
        else phantom = &allocClone(node);
        _phantomMap[&node] = phantom;
        _regMap[&node] = phantom;
        phantom->setParent(nullptr);
        phantom->clearChildren();
        phantom->setData(node.getData());
        phantom->setKey(node.getKey());
        return *phantom;
    }

    void cloneTree(const TreeNode& parent)
    {
        auto phantom = this->phantom(parent);
        assert(phantom);
        _phantomMap.erase(&parent);

        //Recurse through original node's children, clone each child link
        for (auto& child : parent.children())
        {
            auto phantomChild = getRegClone(child);
            if (!phantomChild || phantomChild == this->phantom(child))
            {
                //Unknown node, need to clone it using phantom
                if (!phantomChild) phantomChild = &createPhantom(child);
                cloneTree(child);
            }
            phantom->addChild(*phantomChild);
        }

        for (auto& e : _listeners) const_cast<TreeNode&>(parent).listeners().add(*e);
    }

    /// Unregister a single node without recursing to children.  Returns clone if successful
    TreeNode* unregNodeSingle(const TreeNode& node)
    {
        auto clone = getRegClone(node);
        if (!clone) return nullptr;

        //Clone's parent must not be registered
        assert(!clone->getParent() || !isRegClone(*clone->getParent()),
                    sout()  << "Node can't be unregistered because its parent is registered. Parent Id: "
                            << clone->getParent()->getKey() << " ; Child Id: " << clone->getKey());

        for (auto& e : _listeners) const_cast<TreeNode&>(node).listeners().remove(*e);

        _regMap.erase(&node);
        _phantomMap.erase(&node);
        _unregMap[&node] = clone;

        return clone;
    }

    /// Map from original node to clone
    typedef unordered_map<const TreeNode*, TreeNode*> CloneMap;
    /// Reverse map from clone to original node
    typedef unordered_map<TreeNode*, const TreeNode*> CloneRMap;

    vector<ListenerQueue::Ptr> _listeners;
    CloneMap _cloneMap;
    CloneRMap _cloneRMap;
    CloneMap _regMap;
    CloneMap _phantomMap;
    CloneMap _unregMap;
};

}
