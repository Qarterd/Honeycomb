// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Math/Double.h"

namespace honey
{

/// Random number generator interface
class RandomGen
{
public:
    /// Generate random number between 0 and 2^64-1 inclusive
    virtual uint64 next() = 0;
};

}
