// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

// Windows headers
#ifndef WINVER
    #define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
#endif

#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>
#include <process.h>
#include <assert.h>

namespace honey
{

// Define debug if windows _DEBUG is being used
#ifndef DEBUG
    #ifdef _DEBUG
        #define DEBUG
    #endif
#endif

/// \name Integral types
/// @{
typedef char                int8;
typedef unsigned char       uint8;
typedef short               int16;
typedef unsigned short      uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;
typedef int64               int128;
typedef uint64              uint128;
/// @}

// Disable integer <-> bool conversion performance warning
#pragma warning(disable:4800)

// Disable double -> float precision warning
#pragma warning(disable:4305)
// Disable int -> float precision warning
#pragma warning(disable:4244)

/// \name Real types
/// @{

/// 128 bit float type
typedef long double         float128;
/// @}

/// Platform endian
#define ENDIAN              ENDIAN_LITTLE

/// Align stuct
#define ALIGN(Bytes)        __declspec(align(Bytes))

/// Get current function signature
#define __FUNC__            __FUNCSIG__

/// Force compiler to inline function
#define FORCE_INLINE        __forceinline

}


#ifdef Core_priv
    #include "Honey/Core/platform/priv/Core.h"
#endif


namespace honey
{

/// Conflicting defines used by windows
/// @{
#undef assert
#undef min
#undef max
#undef near
#undef far
/// @}

}