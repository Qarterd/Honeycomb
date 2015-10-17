// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"

namespace honey
{

/// a contiguous region of referenced (not owned) memory
template<class T>
struct Buffer
{
    operator T*() const                         { return data; }
    
    T* data;
    size_t size;
};

}
