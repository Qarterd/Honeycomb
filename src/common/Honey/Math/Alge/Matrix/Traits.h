// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Float.h"
#include "Honey/Math/Double.h"
#include "Honey/Math/Quad.h"
#include "Honey/String/Stream.h"

namespace honey
{

template<class Subclass> class MatrixBase;
template<int Rows, int Cols, class Real = Real, int Options = 0, class Alloc = std::allocator<int8>> class Matrix;
template<int Dim, class Real = Real, int Options = 0, class Alloc = std::allocator<int8>> class Vec;

/// Matrix util
namespace matrix
{
    /// Matrix implementation details
    namespace priv
    {
        template<class Subclass> struct Traits;
    }

    static const int dynamic = -1;

    /// Matrix type options
    struct Option
    {
        enum t
        {
            align_shift = 0,
            align_mask = (1 << 4) - 1,      /// 7 bit alignment stored as log2, so max alignment is 2^15 byte boundary
            vecRow = 1 << 4                 /// Whether vector is a row or column vector
        };

        /// Alignment must be a power of 2
        template<int Align> struct setAlign             : mt::Value<int, (mt::log2Floor<Align>::value & align_mask) << align_shift> {};
        template<int Options> struct getAlign           : mt::Value<int, 1 << ((Options >> align_shift) & align_mask)> {};
    };

    template<class Matrix, int Rows = dynamic, int Cols = dynamic> class Block;
}

/// Vec util
namespace vec
{
    /// Vec implementation details
    namespace priv
    {
        template<class Vec, int Dim = matrix::dynamic> struct Segment;
    }
}

}