// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

#include <unistd.h>

namespace honey
{

/// \name Integral types
/// @{
typedef char                int8;
typedef unsigned char       uint8;
typedef short               int16;
typedef unsigned short      uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long                int64;
typedef unsigned long       uint64;
typedef int64               int128;
typedef uint64              uint128;
/// @}

/// \name Real types
/// @{

/// 128 bit float type
typedef long double         float128;
/// @}

/// Platform endian
#define ENDIAN              ENDIAN_LITTLE

/// Align stuct
#define ALIGN(Bytes)        alignas(Bytes)

/// Get current function signature
#define __FUNC__            __PRETTY_FUNCTION__

//macro conflicts from system headers
#undef assert
#undef minor

}
