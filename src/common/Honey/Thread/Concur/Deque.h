// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Lock/Spin.h"

namespace honey
{
/// Concurrent methods and containers
namespace concur
{

/// Concurrent double-lock deque. Contention is split between the front and back locks.  Automatically expands storage size as needed.
/**
  * Internally maintains a ring-buffer (traversing from head to tail may loop around end of buffer).
  */
template<class Data, class Alloc_ = typename DefaultAllocator<Data>::type>
class Deque : mt::NoCopy
{
public:
    typedef typename Alloc_::template rebind<Data>::other Alloc;

    Deque(int size = 0, const Data& initVal = Data(), const Alloc& alloc = Alloc()) :
        _alloc(alloc),
        _data(nullptr, finalize<Data, Alloc>()),
        _capacity(0),
        _size(0),
        _head(0),
        _tail(0)
    {
        resize(size, initVal);
    }

    ~Deque()                                                { clear(); }

    void resize(int size, const Data& initVal = Data())
    {
        SpinLock::Scoped _(_headLock);
        SpinLock::Scoped __(_tailLock);
        setCapacity(size);
        //Init new data
        int dif = _capacity - _size;
        for (int i = 0; i < dif; ++i) _alloc.construct(_data + ringIndex(_head+_size+i), initVal);
        _size = size;
        _tail = _head;
    }

    /// Insert new element at beginning of list
    void pushFront(const Data& data)
    {
        //At size == 0, head and tail are vying to push the same first spot
        //At size == capacity-1, head and tail are vying to push the same last spot
        //At size == capacity, expansion is needed
        SpinLock::Scoped _(_headLock);
        SpinLock::Scoped __(_tailLock, _size == 0 || _size >= _capacity-1 ? lock::Op::lock : lock::Op::defer);
        if (_size == _capacity) expand();
        _head = ringDec(_head);
        _alloc.construct(_data + _head, data);
        ++_size;
    }

    /// Add new element onto end of list
    void pushBack(const Data& data)
    {
        SpinLock::Scoped headLock(_headLock, lock::Op::defer);
        SpinLock::Scoped tailLock(_tailLock);
        //Lock head first to prevent deadlock
        if (_size == 0 || _size >= _capacity-1) { tailLock.unlock(); headLock.lock(); tailLock.lock(); }
        if (_size == _capacity) expand();
        _alloc.construct(_data + _tail, data);
        _tail = ringInc(_tail);
        ++_size;
    }

    /// Pop element from beginning of list, stores in `data`.  Returns true on success, false if there is no element to pop.
    bool popFront(optional<Data&> data = optnull)
    {
        //At size == 1, head and tail are vying to pop the last spot
        SpinLock::Scoped _(_headLock);
        SpinLock::Scoped __(_tailLock, _size == 1 ? lock::Op::lock : lock::Op::defer);
        if (_size == 0) return false;
        if (data) data = _data[_head];
        _alloc.destroy(_data + _head);
        _head = ringInc(_head);
        --_size;
        return true;
    }

    /// Pop element from end of list, stores in `data`.  Returns true on success, false if there is no element to pop.
    bool popBack(optional<Data&> data = optnull)
    {
        SpinLock::Scoped headLock(_headLock, lock::Op::defer);
        SpinLock::Scoped tailLock(_tailLock);
        if (_size == 1) { tailLock.unlock(); headLock.lock(); tailLock.lock(); }
        if (_size == 0) return false;
        _tail = ringDec(_tail);
        if (data) data = _data[_tail];
        _alloc.destroy(_data + _tail);
        --_size;
        return true;
    }

    /// Remove all elements
    void clear()                                            { while (popBack()); }

    /// Number of elements in list
    int size() const                                        { return _size; }

    /// Check if deque does not contain any elements
    bool empty() const                                      { return size() == 0; }

private:
    int ringIndex(int index) const                          { return index % _capacity; }
    int ringInc(int index) const                            { return index >= _capacity - 1 ? 0 : index + 1; }
    int ringDec(int index) const                            { return index <= 0 ? _capacity - 1 : index - 1; }

    void setCapacity(int capacity)
    {
        if (capacity == _capacity) return;
        //Get size (active element count) of new array, may be smaller than old
        int size = capacity < _size ? capacity : _size.load();
        //Alloc new array
        Data* data = nullptr;
        if (capacity > 0)
        {
            data = _alloc.allocate(capacity);
            //Copy active elements to new array (new head is at 0)
            if (_size > 0)
            {
                int copyTail = ringIndex(_head + size);
                if (copyTail > _head)
                    //Contiguous region
                    std::copy(_data.get() + _head, _data + copyTail, data);
                else
                {
                    //Split region, loops around end
                    std::copy(_data.get() + _head, _data + _capacity, data);
                    std::copy(_data.get(), _data + copyTail, data + (_capacity - _head));
                }
            }
        }
        //Destroy any active elements that don't fit into new array
        int dif = _size - size;
        for (int i = 0; i < dif; ++i) _alloc.destroy(_data + ringIndex(_head+size+i));
        //Set new array
        _data = data;
        _capacity = capacity;
        _size = size;
        _head = 0;
        _tail = size;
    }

    void expand()                                           { setCapacity(_capacity + _capacity/2 + 1); } //Expand 50%

    Alloc               _alloc;
    UniquePtr<Data, finalize<Data,Alloc>> _data;
    int                 _capacity;
    atomic::Var<int>    _size;
    int                 _head;
    int                 _tail;
    SpinLock            _headLock;
    SpinLock            _tailLock;
};

} }
