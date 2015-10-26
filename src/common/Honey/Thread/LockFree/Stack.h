// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/LockFree/FreeList.h"

namespace honey { namespace lockfree
{

/// Lock-free FILO stack. Uses auto-expanding freelist allocator so memory is only reclaimed upon destruction.
template<class T>
class Stack : mt::NoCopy
{
    struct Node
    {
        T val;
        Node* next;
    };
    
    typedef FreeList<Node> FreeList;
    typedef typename FreeList::TaggedHandle TaggedHandle;
    
public:
    typedef T value_type;
    
    Stack(szt capacity = 0)                         : _freeList(capacity), _top(TaggedHandle()), _size(0) {}
    
    ~Stack()                                        { clear(); }
    
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
            old = _top.load(atomic::Order::acquire);
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
            if (!(old = _top.load(atomic::Order::acquire))) return false;
            node = _freeList.deref(old);
        } while (!_top.cas(TaggedHandle(_freeList.handle(node->next), old.nextTag()), old));
        --_size;
        
        if (val) val = move(node->val);
        _freeList.destroy(node);
        return true;
    }
    
    /// Get a copy of the top element. Returns true on success, false if there is no element.
    bool top(T& val)
    {
        //loop to ensure val we read is consistent with top, otherwise we could end up returning a destroyed value
        TaggedHandle top;
        do
        {
            if (!(top = _top.load(atomic::Order::acquire))) return false;
            val = _freeList.deref(top)->val;
        } while (top != _top.load(atomic::Order::acquire));
        return true;
    }
    
    /// Remove all elements
    void clear()                                    { while (pop()); }
    
    bool empty() const                              { return !_size; }
    szt size() const                                { return _size; }
    
private:
    FreeList _freeList;
    Atomic<TaggedHandle> _top;
    Atomic<szt> _size;
};

} }
