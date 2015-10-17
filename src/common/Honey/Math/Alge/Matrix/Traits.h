// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"
#include "Honey/String/Stream.h"

namespace honey
{

template<class Subclass> class MatrixBase;
template<sdt Rows, sdt Cols, class Real = Real, int Options = 0, class Alloc = std::allocator<int8>> class Matrix;
template<sdt Dim, class Real = Real, int Options = 0, class Alloc = std::allocator<int8>> class Vec;

/// Matrix util
namespace matrix
{
    /// Matrix implementation details
    namespace priv
    {
        template<class Subclass> struct Traits;
    }

    static const sdt dynamic = -1;

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
        template<szt Align> struct setAlign             : mt::Value<int, (mt::log2Floor<Align>::value & align_mask) << align_shift> {};
        template<int Options> struct getAlign           : mt::Value<szt, 1 << ((Options >> align_shift) & align_mask)> {};
    };

    template<class Matrix, sdt Rows = dynamic, sdt Cols = dynamic> class Block;
}

/// Vec util
namespace vec
{
    /// Vec implementation details
    namespace priv
    {
        template<class Vec, sdt Dim = matrix::dynamic> struct Segment;
    }
}

}