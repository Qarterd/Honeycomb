// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/LockFree/FreeList.h"

namespace honey { namespace lockfree
{

/// Lock-free FILO stack. Automatically expands to accommodate new elements.
template<class T>
class Stack : mt::NoCopy
{
    struct Node
    {
        T val;
        Node* next;
    };
    
public:
    typedef T value_type;
    typedef FreeList<Node> FreeList;
    typedef typename FreeList::TaggedHandle TaggedHandle;
    
    Stack(szt capacity = 0)                         : _freeList(capacity), _top(TaggedHandle()), _size(0) {}

    /// Ensure that enough storage is allocated for a number of elements
    void reserve(szt capacity)                      { _freeList.reserve(capacity); }
    szt capacity() const                            { return _freeList.capacity(); }
    
    /// Add new element onto top of stack
    template<class T_>
    void push(T_&& val)
    {
        Node* node = _freeList.construct(forward<T_>(val), nullptr);
        
        //Attach node as top
        TaggedHandle old;
        do
        {
            old = _top;
            node->next = old ? _freeList.deref(old) : nullptr;
        } while (!_top.cas(TaggedHandle(_freeList.handle(node), old.nextTag()), old));
        ++_size;
    }
    
    /// Remove element from top of stack, stores in `val`. Returns true on success, false if there is no element to pop.
    bool pop(optional<T&> val = optnull)
    {
        //Detach node from top
        TaggedHandle old;
        Node* node;
        do
        {
            if (!(old = _top)) return false;
            node = _freeList.deref(old);
        } while (!_top.cas(TaggedHandle(_freeList.handle(node->next), old.nextTag()), old));
        --_size;
        
        if (val) val = move(node->val);
        _freeList.destroy(node);
        return true;
    }
    
    bool empty() const                              { return !_size; }
    szt size() const                                { return _size; }
    
private:
    FreeList _freeList;
    Atomic<TaggedHandle> _top;
    Atomic<szt> _size;
};

} }
