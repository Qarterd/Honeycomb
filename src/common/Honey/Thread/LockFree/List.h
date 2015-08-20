// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Memory/UniquePtr.h"
#include "Honey/Thread/LockFree/Mem.h"
#include "Honey/Thread/LockFree/Backoff.h"

namespace honey
{
/// Lock-free methods and containers
namespace lockfree
{

/// Lock-free doubly-linked list.
/**
  * Based on the paper: "Lock-free deques and doubly linked lists", Sundell, et al. - 2008
  *
  * \tparam Data        Container element type
  * \tparam Alloc       Node allocator.  Pointers returned by allocator must be at least 2-byte aligned.
  * \tparam Backoff     Backoff algorithm to reduce contention
  * \tparam iterMax     Max number of iterator instances per thread
  */
template<class Data_, class Alloc_ = typename DefaultAllocator<Data_>::type, class Backoff = Backoff, int8 iterMax = 2>
class List : MemConfig, mt::NoCopy
{
    template<class> friend class Mem;
public:
    typedef Data_ Data;
    
private:
    struct Node : MemNode
    {
        ///Combines node pointer and delete mark in one Cas-able integer
        struct Link : MemLink<Node>
        {
            using MemLink<Node>::data;
            static const intptr_t d_mask = 1;
            static const intptr_t ptr_mask = ~d_mask;

            Link()                                          { data = 0; }
            Link(Node* ptr, bool d = false)                 { data = reinterpret_cast<intptr_t>(ptr) | (intptr_t)d; }
            Node* ptr() const                               { return reinterpret_cast<Node*>(data & ptr_mask); }
            bool d() const                                  { return data & d_mask; }
            bool cas(const Link& val, const Link& old)      { return data.cas(val.data, old.data); }

            bool operator==(const Link& rhs)                { return data == rhs.data; }
            bool operator!=(const Link& rhs)                { return !operator==(rhs); }
        };

        Link next;
        Link prev;
        Data data;
    };
    typedef typename Node::Link Link;

public:
    typedef typename Alloc_::template rebind<Node>::other Alloc;

    /**
      * \param threadMax    Max number of threads that can access this container. Use a thread pool so the threads have a longer life cycle than this container.
      * \param alloc
      */
    List(int threadMax = 8, const Alloc& alloc = Alloc()) :
        _alloc(alloc),
        _mem(*this, threadMax),
        _size(0)
    {
        _mem.storeRef(_head, &createNode(Data()));
        _mem.storeRef(_tail, &createNode(Data()));
        _mem.storeRef(_head.ptr()->next, _tail);
        _mem.storeRef(_tail.ptr()->prev, _head);
        _mem.releaseRef(*_head.ptr());
        _mem.releaseRef(*_tail.ptr());
    }

    ~List()
    {
        clear();
        _mem.deleteNode(*_head.ptr());
        _mem.deleteNode(*_tail.ptr());
    }

    /// Insert new element at beginning of list
    void pushFront(const Data& data)
    {
        Node& node = createNode(data);
        Node* prev = _mem.deRefLink(_head);
        Node* next = _mem.deRefLink(prev->next);
        _backoff.reset();
        while (true)
        {
            _mem.storeRef(node.prev, prev);
            _mem.storeRef(node.next, next);
            if (_mem.casRef(prev->next, &node, next)) break;
            _mem.releaseRef(*next);
            next = _mem.deRefLink(prev->next);
            _backoff.inc();
            _backoff.wait();
        }
        ++_size;
        _mem.releaseRef(*prev);
        pushEnd(node, *next);
    }

    /// Add new element onto end of list
    void pushBack(const Data& data)
    {
        Node& node = createNode(data);
        Node* next = _mem.deRefLink(_tail);
        Node* prev = _mem.deRefLink(next->prev);
        _backoff.reset();
        while (true)
        {
            _mem.storeRef(node.prev, prev);
            _mem.storeRef(node.next, next);
            if (_mem.casRef(prev->next, &node, next)) break;
            prev = &correctPrev(*prev, *next);
            _backoff.inc();
            _backoff.wait();
        }
        ++_size;
        _mem.releaseRef(*prev);
        pushEnd(node, *next);
    }

    /// Pop element from beginning of list, stores in `data`.  Returns true on success, false if there is no element to pop.
    bool popFront(optional<Data&> data = optnull)
    {
        Node* prev = _mem.deRefLink(_head);
        _backoff.reset();
        while (true)
        {
            Node* node = _mem.deRefLink(prev->next);
            if (node == _tail.ptr())
            {
                _mem.releaseRef(*node);
                _mem.releaseRef(*prev);
                return false;
            }
            bool next_d = node->next.d();
            Node* next = _mem.deRefLink(node->next);
            if (next_d)
            {
                setMark(node->prev);
                _mem.casRef(prev->next, next, node);
                _mem.releaseRef(*next);
                _mem.releaseRef(*node);
                continue;
            }
            if (_mem.casRef(node->next, Link(next,true), next))
            {
                --_size;
                prev = &correctPrev(*prev, *next);
                _mem.releaseRef(*prev);
                _mem.releaseRef(*next);
                if (data) data = node->data;
                _mem.releaseRef(*node);
                _mem.deleteNode(*node);
                break;
            }
            _mem.releaseRef(*next);
            _mem.releaseRef(*node);
            _backoff.inc();
            _backoff.wait();
        }
        return true;
    }

    /// Pop element from end of list, stores in `data`.  Returns true on success, false if there is no element to pop.
    bool popBack(optional<Data&> data = optnull)
    {
        Node* next = _mem.deRefLink(_tail);
        Node* node = _mem.deRefLink(next->prev);
        _backoff.reset();
        while (true)
        {
            if (node->next != Link(next))
            {
                node = &correctPrev(*node, *next);
                continue;
            }
            if (node == _head.ptr())
            {
                _mem.releaseRef(*node);
                _mem.releaseRef(*next);
                return false;
            }
            if (_mem.casRef(node->next, Link(next,true), next))
            {
                --_size;
                Node* prev = _mem.deRefLink(node->prev);
                prev = &correctPrev(*prev, *next);
                _mem.releaseRef(*prev);
                _mem.releaseRef(*next);
                if (data) data = node->data;
                _mem.releaseRef(*node);
                _mem.deleteNode(*node);
                break;
            }
            _backoff.inc();
            _backoff.wait();
        }
        return true;
    }

    /// Iterator
    /**
      * An iterator instance is not thread-safe; it can't be shared between threads without a lock. \n
      * Each iterator needs a thread-local node reference, so the number of iterator instances allowed is limited by `iterMax`.
      */
    template<class Data>
    class Iter_
    {
        friend class List;

    public:
        typedef std::bidirectional_iterator_tag     iterator_category;
        typedef Data                                value_type;
        typedef sdt                                 difference_type;
        typedef Data*                               pointer;
        typedef Data&                               reference;
        
        Iter_()                                             : _list(nullptr), _cur(nullptr) {}

        Iter_(const List& list, bool end) :
            _list(const_cast<List*>(&list)),
            _cur(!end ? _list->_head.ptr() : _list->_tail.ptr())
        {
            _list->_mem.ref(*_cur);
        }

        Iter_(const Iter_& it)                              : _cur(nullptr) { operator=(it); }

        ~Iter_()
        {
            if (_cur) _list->_mem.releaseRef(*_cur);
        }

        /// Copy iterator, must reference copied cursor
        Iter_& operator=(const Iter_& it)
        {
            if (_cur) _list->_mem.releaseRef(*_cur);
            _list = it._list;
            _cur = it._cur;
            if (_cur) _list->_mem.ref(*_cur);
            return *this;
        }

        Iter_& operator++()
        {
            while (true)
            {
                if (_cur == _list->_tail.ptr()) break;
                Node* next = _list->_mem.deRefLink(_cur->next);
                bool d = next->next.d();
                if (d && _cur->next != Link(next, true))
                {
                    _list->setMark(next->prev);
                    _list->_mem.casRef(_cur->next, next->next.ptr(), next);
                    _list->_mem.releaseRef(*next);
                    continue;
                }
                _list->_mem.releaseRef(*_cur);
                _cur = next;
                if (!d) break;
            }
            return *this;
        }

        Iter_& operator--()
        {
            while (true)
            {
                if (_cur == _list->_head.ptr()) break;
                Node* prev = _list->_mem.deRefLink(_cur->prev);
                if (prev->next == Link(_cur) && !_cur->next.d())
                {
                    _list->_mem.releaseRef(*_cur);
                    _cur = prev;
                    break;
                }
                else if (_cur->next.d())
                {
                    _list->_mem.releaseRef(*prev);
                    operator++();
                }
                else
                {
                    prev = &_list->correctPrev(*prev, *_cur);
                    _list->_mem.releaseRef(*prev);
                }
            }
            return *this;
        }

        Iter_ operator++(int)                               { auto tmp = *this; ++*this; return tmp; }
        Iter_ operator--(int)                               { auto tmp = *this; --*this; return tmp; }

        bool operator==(const Iter_& rhs) const             { return _cur == rhs._cur; }
        bool operator!=(const Iter_& rhs) const             { return !operator==(rhs); }
        
        reference operator*() const                         { return _cur->data; }
        pointer operator->() const                          { return &operator*(); }

        /// Returns true if iterator points to valid element that has not been deleted
        bool valid() const                                  { return !_cur->next.d(); }

    private:
        List* _list;
        Node* _cur;
    };

    typedef Iter_<Data> Iter;
    typedef Iter_<const Data> ConstIter;

    /// Get iterator to the beginning of the list
    ConstIter begin() const                                 { return ++ConstIter(*this, false); }
    Iter begin()                                            { return ++Iter(*this, false); }

    /// Get iterator to the end of the list
    ConstIter end() const                                   { return ConstIter(*this, true); }
    Iter end()                                              { return Iter(*this, true); }

    /// Reverse iterator
    template<class Data>
    class IterR_
    {
        friend class List;

    public:
        typedef Iter_<Data> Iter;

        typedef std::bidirectional_iterator_tag     iterator_category;
        typedef Data                                value_type;
        typedef sdt                                 difference_type;
        typedef Data*                               pointer;
        typedef Data&                               reference;

        IterR_(Iter& it = Iter())                           : _it(it) {}

        IterR_& operator++()                                { --_it; return *this; }
        IterR_& operator--()                                { ++_it; return *this; }
        IterR_ operator++(int)                              { auto tmp = *this; ++*this; return tmp; }
        IterR_ operator--(int)                              { auto tmp = *this; --*this; return tmp; }

        bool operator==(const IterR_& rhs) const            { return _it == rhs._it; }
        bool operator!=(const IterR_& rhs) const            { return !operator==(rhs); }

        reference operator*() const                         { return *_it; }
        pointer operator->() const                          { return _it.operator->(); }

        bool valid() const                                  { return _it.valid(); }

    private:
        Iter _it;
    };

    typedef IterR_<Data> IterR;
    typedef IterR_<const Data> ConstIterR;

    /// Get reverse iterator to the end of the list
    ConstIterR beginR() const                               { return --end(); }
    IterR beginR()                                          { return --end(); }

    /// Get reverse iterator to the beginning of the list
    ConstIterR endR() const                                 { return ConstIter(*this, false); }
    IterR endR()                                            { return Iter(*this, false); }

    /// Get reference to front element.  Returns true on success, false if there is no element.
    bool front(Data& data)
    {
        Iter it = begin();
        if (it == end() || !it.valid()) return false;
        data = *it;
        return true;
    }

    /// Get reference to back element.  Returns true on success, false if there is no element.
    bool back(Data& data)
    {
        IterR it = beginR();
        if (it == endR() || !it.valid()) return false;
        data = *it;
        return true;
    }

    /// Insert element before iterator position.  Returns iterator pointing to new element.
    Iter insert(const Iter& it, const Data& data)
    {
        Iter pos = it;
        assert(pos._cur != _head.ptr());

        Node& node = createNode(data);
        Node* prev = _mem.deRefLink(pos._cur->prev);
        Node* next = nullptr;
        _backoff.reset();
        while (true)
        {
            while (pos._cur->next.d())
            {
                ++pos;
                prev = &correctPrev(*prev, *pos._cur);
            }
            next = pos._cur;
            _mem.storeRef(node.prev, prev);
            _mem.storeRef(node.next, next);
            if (_mem.casRef(prev->next, &node, pos._cur)) break;
            prev = &correctPrev(*prev, *pos._cur);
            _backoff.inc();
            _backoff.wait();
        }
        ++_size;
        _mem.releaseRef(*prev);
        //correctPrev takes control of our node ref, so add another ref then release the node returned.
        _mem.ref(node);
        _mem.releaseRef(correctPrev(node, *next));
        _mem.releaseRef(*next);
        pos._cur = &node;
        return pos;
    }

    /// Erase element at iterator position, store erased element in `data`, and advance iterator. Returns true if this thread erased and stored the element, false if already erased.
    bool erase(Iter& it, optional<Data&> data = optnull)
    {
        bool erased = false;
        Node* node = it._cur;
        assert(node != _head.ptr() && node != _tail.ptr());
        while (true)
        {
            bool next_d = it._cur->next.d();
            Node* next = _mem.deRefLink(it._cur->next);
            if (next_d)
            {
                _mem.releaseRef(*next);
                break;
            }
            if (node->next.cas(Link(next, true), next))
            {
                erased = true;
                --_size;
                Node* prev = nullptr;
                while (true)
                {
                    bool bPrev = node->prev.d();
                    prev = _mem.deRefLink(node->prev);
                    if (bPrev || node->prev.cas(Link(prev, true), prev)) break;
                    _mem.releaseRef(*prev);
                }
                prev = &correctPrev(*prev, *next);
                _mem.releaseRef(*prev);
                _mem.releaseRef(*next);
                if (data) data = node->data;
                _mem.deleteNode(*node);
                break;
            }
            _mem.releaseRef(*next);
        }
        ++it;
        return erased;
    }

    /// Remove all elements
    void clear()                                            { for (Iter it = begin(); it != end();) erase(it); }

    /// Number of elements in list
    szt size() const                                        { sdt size = _size; return size > 0 ? size : 0; } //Size can be less than 0 temporarily because of concurrency

private:
    /// Override from MemConfig
    static const int linkMax = 2;
    /// Override from MemConfig
    static const int linkDelMax = linkMax;
    /// Override from MemConfig
    static const int8 tlrefMax = 5 + iterMax;

    /// Override from MemConfig
    Alloc& getAlloc()                                       { return _alloc; }

    /// Override from MemConfig
    void cleanUpNode(Node& node)
    {
        while (true)
        {
            Node* prev = _mem.deRefLink(node.prev);
            if (!prev) break;
            if (!prev->prev.d())
            {
                _mem.releaseRef(*prev);
                break;
            }
            Node* prev2 = _mem.deRefLink(prev->prev);
            _mem.casRef(node.prev, Link(prev2, true), Link(prev, true));
            _mem.releaseRef(*prev2);
            _mem.releaseRef(*prev);
        }
        while (true)
        {
            Node* next = _mem.deRefLink(node.next);
            if (!next) break;
            if (!next->next.d())
            {
                _mem.releaseRef(*next);
                break;
            }
            Node* next2 = _mem.deRefLink(next->next);
            _mem.casRef(node.next, Link(next2, true), Link(next, true));
            _mem.releaseRef(*next2);
            _mem.releaseRef(*next);
        }
    }

    /// Override from MemConfig
    void terminateNode(Node& node, bool concurrent)
    {
        if (!concurrent)
        {
            _mem.storeRef(node.prev, Link(nullptr, true));
            _mem.storeRef(node.next, Link(nullptr, true));
        }
        else
        {
            _mem.casRef(node.prev, Link(nullptr, true), Link(node.prev));
            _mem.casRef(node.next, Link(nullptr, true), Link(node.next));
        }
    }

    Node& createNode(const Data& data)
    {
        Node& node = _mem.createNode();
        assert((reinterpret_cast<intptr_t>(&node) & ~Link::ptr_mask) == 0, "Pointer not 2-byte aligned, bits found outside mask.");
        node.prev = Link();
        node.next = Link();
        node.data = data;
        return node;
    }

    /// Set delete mark
    void setMark(Link& link)
    {
        while (true)
        {
            Link old = link;
            if (old.d() || link.cas(Link(old.ptr(), true), old)) break;
        }
    }

    /// End of push method
    void pushEnd(Node& node, Node& next)
    {
        Node* pNode = &node;
        _backoff.reset();
        while (true)
        {
            Link link = next.prev;
            if (link.d() || node.next != Link(&next)) break;
            if (_mem.casRef(next.prev, &node, link))
            {
                if (node.prev.d())
                    pNode = &correctPrev(node, next);
                break;
            }
            _backoff.inc();
            _backoff.wait();
        }
        _mem.releaseRef(next);
        _mem.releaseRef(*pNode);
    }

    /// Update the prev pointer of `node` using `prev` as a suggestion.  Returns a possible previous node.  May release ref to `prev`.
    Node& correctPrev(Node& prev_, Node& node)
    {
        Node* prev = &prev_;
        Node* lastLink = nullptr;
        _backoffCp.reset();
        while (true)
        {
            Link link = node.prev;
            if (link.d())
            {
                //node was deleted while correcting, prev may have advanced past node, so undo the last step
                if (lastLink)
                {
                    _mem.releaseRef(*prev);
                    prev = lastLink;
                    lastLink = nullptr;
                }
                break;
            }
            bool prev2_d = prev->next.d();
            Node* prev2 = _mem.deRefLink(prev->next);
            if (prev2_d)
            {
                if (lastLink)
                {
                    setMark(prev->prev);
                    _mem.casRef(lastLink->next, prev2, prev);
                    _mem.releaseRef(*prev2);
                    _mem.releaseRef(*prev);
                    prev = lastLink;
                    lastLink = nullptr;
                    continue;
                }
                _mem.releaseRef(*prev2);
                prev2 = _mem.deRefLink(prev->prev);
                _mem.releaseRef(*prev);
                prev = prev2;
                continue;
            }
            if (prev2 != &node)
            {
                if (lastLink) _mem.releaseRef(*lastLink);
                lastLink = prev;
                prev = prev2;
                continue;
            }
            _mem.releaseRef(*prev2);
            if (_mem.casRef(node.prev, prev, link))
            {
                if (prev->prev.d()) continue;
                break;
            }
            _backoffCp.inc();
            _backoffCp.wait();
        }
        if (lastLink) _mem.releaseRef(*lastLink);
        return *prev;
    }

    Alloc                   _alloc;
    Mem<List>               _mem;
    Link                    _head;
    Link                    _tail;
    atomic::Var<sdt>        _size;
    Backoff                 _backoff;
    Backoff                 _backoffCp; ///< Backoff for correctPrev
};

} }
