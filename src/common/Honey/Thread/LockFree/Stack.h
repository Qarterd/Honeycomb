// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/LockFree/FreeList.h"

namespace honey { namespace lockfree
{

/// Lock-free stack. Automatically expands to accommodate new elements.
template<class T>
class Stack : mt::NoCopy
{
public:
    typedef T value_type;
    
    Stack(szt capacity = 0)                         : _freeList(capacity) {}

    /// Ensure that enough storage is allocated for a number of elements
    void reserve(szt capacity)                      { _freeList.reserve(capacity); }
    
    
private:
    FreeList<T> _freeList;
};

} }
