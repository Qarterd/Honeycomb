// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/LockFree/FreeList.h"

namespace honey { namespace lockfree
{

/// Lock-free FIFO queue. Uses internal free-list allocator, automatically expands to accommodate new elements.
/**
  * Based on the paper: "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms", Michael, Scott - 1996
  */
template<class T>
class Queue : mt::NoCopy
{
    struct Node
    {
        typedef typename FreeList<Node*>::TaggedHandle TaggedHandle;
        
        //init without overwriting previous tag
        template<class T_>
        Node(T_&& val)                              : val(forward<T_>(val)) { reinterpret_cast<TaggedHandle&>(next).handle() = nullptr; }
        //full init
        template<class T_>
        Node(T_&& val, TaggedHandle next)           : val(forward<T_>(val)), next(next) {}
        
        T val;
        Atomic<TaggedHandle> next;
    };
    
    typedef FreeList<Node> FreeList;
    typedef typename FreeList::TaggedHandle TaggedHandle;
    
public:
    typedef T value_type;
    
    Queue(szt capacity = 0) :
        _freeList(capacity),
        _head(TaggedHandle(_freeList.handle(_freeList.construct(T(), TaggedHandle())), 0)),
        _tail(_head),
        _size(0) {}

    ~Queue()                                        { clear(); _freeList.destroy(_freeList.deref(_head)); }
    
    /// Ensure that enough storage is allocated for a number of elements
    void reserve(szt capacity)                      { _freeList.reserve(capacity); }
    szt capacity() const                            { return _freeList.capacity(); }
    
    /// Add new element onto the end of the queue
    template<class T_>
    void push(T_&& val)
    {
        Node* node = _freeList.construct(forward<T_>(val));
        
        TaggedHandle tail;
        while (true)
        {
            tail = _tail;
            TaggedHandle next = _freeList.deref(tail)->next;
            //ensure that tail and next are consistent
            if (tail != _tail) continue;
            //check if tail isn’t last
            if (next)
            {
                //tail isn’t at the last element, try to move tail forward
                _tail.cas(TaggedHandle(next, tail.nextTag()), tail);
                continue;
            }
            //try to add the element onto the end of the list
            if (_freeList.deref(tail)->next.cas(TaggedHandle(_freeList.handle(node), next.nextTag()), next)) break; //success
        }
        //try to relocate tail to the inserted element
        _tail.cas(TaggedHandle(_freeList.handle(node), tail.nextTag()), tail);
        ++_size;
    }
    
    /// Remove oldest element from the queue, stores in `val`. Returns true on success, false if there is no element to pop.
    bool pop(optional<T&> val = optnull)
    {
        TaggedHandle head;
        while (true)
        {
            head = _head;
            TaggedHandle tail = _tail;
            TaggedHandle next = _freeList.deref(head)->next;
            //ensure that head, tail and next are consistent
            if (head != _head) continue;
            //check if queue is empty or tail isn’t last
            if (head.handle() == tail)
            {
                if (!next) return false; //queue is empty
                //tail isn’t at the last element, try to move tail forward
                _tail.cas(TaggedHandle(next, tail.nextTag()), tail);
                continue;
            }
            //ensure we have a next, it's possible that the list was empty and then changed before reading tail
            if (!next) continue;
            //tail is in position, read the value before cas, otherwise another pop can destroy next
            if (val) val = _freeList.deref(next)->val; //copy val (not move) as there may be concurrent pop attempts
            //try to move head forward
            if (_head.cas(TaggedHandle(next, head.nextTag()), head)) break; //success
        }
        --_size;
        
        _freeList.destroy(_freeList.deref(head));
        return true;
    }
    
    /// Get a copy of the next element that will be popped. Returns true on success, false if there is no element.
    bool front(T& val)
    {
        while (true)
        {
            TaggedHandle head = _head;
            TaggedHandle tail = _tail;
            TaggedHandle next = _freeList.deref(head)->next;
            //ensure that head, tail and next are consistent
            if (head != _head) continue;
            //check if queue is empty
            if (head.handle() == tail && !next) return false;
            //ensure we have a next, it's possible that the list was empty and then changed before reading tail
            if (!next) continue;
            //ensure val we read is consistent with head, otherwise we could end up returning a destroyed value
            val = _freeList.deref(next)->val;
            if (head == _head) return true;
        }
    }

    /// Get a copy of the last element. Returns true on success, false if there is no element.
    bool back(T& val)
    {
        while (true)
        {
            TaggedHandle head = _head;
            TaggedHandle tail = _tail;
            TaggedHandle next = _freeList.deref(tail)->next;
            //ensure that tail and next are consistent
            if (tail != _tail) continue;
            //check if tail isn’t last
            if (next)
            {
                //tail isn’t at the last element, try to move tail forward
                _tail.cas(TaggedHandle(next, tail.nextTag()), tail);
                continue;
            }
            //check if queue is empty
            if (head.handle() == tail) return false;
            //ensure val we read is consistent with head and tail, otherwise we could end up returning a destroyed value
            val = _freeList.deref(tail)->val;
            if (head == _head && tail == _tail) return true;
        }
    }
    
    /// Remove all elements
    void clear()                                    { while (pop()); }
    
    bool empty() const                              { return !_size; }
    szt size() const                                { return _size; }
    
private:
    FreeList _freeList;
    Atomic<TaggedHandle> _head;
    Atomic<TaggedHandle> _tail;
    Atomic<szt> _size;
};

} }
