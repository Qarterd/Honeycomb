// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Math/Numeral.h"

namespace honey
{

/// Endian (byte order) types
enum class Endian
{
    little, ///< low byte first
    big     ///< high byte first
};

/// Bit util common to any endian type. Use through class BitOp.
struct BitOpCommon
{
    /// Rotate integer bits cyclically to the left
    template<class T>
    static T rotLeft(const T v, const int n)
    {
        typedef typename std::make_unsigned<T>::type Unsigned;
        return (T)(((Unsigned)v << n) | ((Unsigned)v >> (Numeral<T>::sizeBits-n)));
    }
    /// Rotate integer bits cyclically to the right
    template<class T>
    static T rotRight(const T v, const int n)               { return rotLeft(v, Numeral<T>::sizeBits-n); }

    /// Reverse order of bytes in an unsigned integer
    static uint8 swap(const uint8 v)                        { return v; }
    static uint16 swap(const uint16 v)                      { return rotLeft(v,8); }
    static uint32 swap(const uint32 v)                      { return (rotLeft(v,8) & 0x00FF00FFU) | (rotLeft(v,24) & 0xFF00FF00U); }
    static uint64 swap(const uint64 v)
    {
        return  (rotLeft(v, 8) & 0x000000FF000000FFULL) |
                (rotLeft(v,24) & 0x0000FF000000FF00ULL) |
                (rotLeft(v,40) & 0x00FF000000FF0000ULL) |
                (rotLeft(v,56) & 0xFF000000FF000000ULL);
    }
    /// Reverse order of bytes in a signed integer
    template<class T>
    static T swap(const T v)
    {
        typedef typename std::make_unsigned<T>::type Unsigned;
        return (T)swap((Unsigned)v);
    }

    /// Retrieve high bytes from integer
    static uint8    high(const uint16 v)                    { return v >> 8; }
    static uint16   high(const uint32 v)                    { return v >> 16; }
    static uint32   high(const uint64 v)                    { return v >> 32; }

    /// Retrieve low bytes from integer
    static uint8    low(const uint16 v)                     { return (uint8)v; }
    static uint16   low(const uint32 v)                     { return (uint16)v; }
    static uint32   low(const uint64 v)                     { return (uint32)v; }

    /// Convert smaller integer parts into a full integer
    static uint32 fromParts(const uint16 hi, const uint16 lo)   { return (uint32)lo | ((uint32)hi << 16); }
    static uint64 fromParts(const uint32 hi, const uint32 lo)   { return (uint64)lo | ((uint64)hi << 32); }
    
    template<class UInt>
    static UInt fromPartsLittle(const uint8* p)             { UInt val = 0; mt::for_<0, sizeof(UInt)>([&](int i) { val |= (UInt)p[i] << i*8; }); return val; }
    template<class UInt>
    static UInt fromPartsBig(const uint8* p)                { UInt val = 0; mt::for_<0, sizeof(UInt)>([&](int i) { val |= (UInt)p[i] << (sizeof(UInt)-1-i)*8; }); return val; }

    template<class UInt>
    static void toPartsLittle(const UInt v, uint8* p)       { mt::for_<0, sizeof(UInt)>([&](int i) { p[i] = (uint8)(v >> i*8); }); }
    template<class UInt>
    static void toPartsBig(const UInt v, uint8* p)          { mt::for_<0, sizeof(UInt)>([&](int i) { p[i] = (uint8)(v >> (sizeof(UInt)-1-i)*8); }); }

    /// Get number of non-zero bits in unsigned integer
    static int popCount(uint32 x)
    {
        x -= (x >> 1) & 0x55555555;
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0f0f0f0f;
        x += x >> 8;
        x += x >> 16;
        return x & 0x0000003f;
    }
    static int popCount(uint64 x)
    {
        x -= (x >> 1) & 0x5555555555555555;             //put count of each 2 bits into those 2 bits
        x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333); //put count of each 4 bits into those 4 bits
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;        //put count of each 8 bits into those 8 bits
        x += x >>  8;  //put count of each 16 bits into their lowest 8 bits
        x += x >> 16;  //put count of each 32 bits into their lowest 8 bits
        x += x >> 32;  //put count of each 64 bits into their lowest 8 bits
        return x & 0x7f;
    }
    
    /// Check if unsigned integer is a power of two
    template<class UInt>
    static bool isPow2(UInt x)                              { return !((x-1) & x); }
    /// Calc nearest power of two <= unsigned integer
    template<class UInt>
    static UInt pow2Floor(UInt x)                           { return isPow2(x) ? x : pow2Ceil(x) >> 1; }
    /// Calc nearest power of two >= unsigned integer
    static uint32 pow2Ceil(uint32 x)                        { --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; return ++x; }
    static uint64 pow2Ceil(uint64 x)                        { --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; x|=x>>32; return ++x; }

    /// Calc log base 2 of unsigned integer, rounded down to nearest integer. Returns -1 if x is zero.
    static int log2Floor(uint32 x)                          { x|=(x>>1); x|=(x>>2); x|=(x>>4); x|=(x>>8); x|=(x>>16); return x ? popCount(x>>1) : -1; }
    static int log2Floor(uint64 x)                          { x|=(x>>1); x|=(x>>2); x|=(x>>4); x|=(x>>8); x|=(x>>16); x|=(x>>32); return x ? popCount(x>>1) : -1; }
    /// Calc log base 2 of unsigned integer, rounded up to nearest integer. Returns -1 if x is zero.
    static int log2Ceil(uint32 x)                           { int32 y=(x&(x-1)); y|=-y; y>>=(32-1); x|=(x>>1); x|=(x>>2); x|=(x>>4); x|=(x>>8); x|=(x>>16); return x ? popCount(x>>1)-y : -1; }
    static int log2Ceil(uint64 x)                           { int64 y=(x&(x-1)); y|=-y; y>>=(64-1); x|=(x>>1); x|=(x>>2); x|=(x>>4); x|=(x>>8); x|=(x>>16); x|=(x>>32); return x ? popCount(x>>1)-int(y) : -1; }

    /// Reverse order of bits in an unsigned integer
    static uint8 reverse(uint8 v)
    {
        v = (v & 0xF0) >> 4 | (v & 0x0F) << 4;
        v = (v & 0xCC) >> 2 | (v & 0x33) << 2;
        v = (v & 0xAA) >> 1 | (v & 0x55) << 1;
        return v;
    }
    static uint16 reverse(uint16 v)
    {
        v = (v & 0xFF00) >> 8 | (v & 0x00FF) << 8;
        v = (v & 0xF0F0) >> 4 | (v & 0x0F0F) << 4;
        v = (v & 0xCCCC) >> 2 | (v & 0x3333) << 2;
        v = (v & 0xAAAA) >> 1 | (v & 0x5555) << 1;
        return v;
    }
    static uint32 reverse(uint32 v)
    {
        v = (v & 0xFFFF0000) >> 16 | (v & 0x0000FFFF) << 16;
        v = (v & 0xFF00FF00) >> 8  | (v & 0x00FF00FF) << 8;
        v = (v & 0xF0F0F0F0) >> 4  | (v & 0x0F0F0F0F) << 4;
        v = (v & 0xCCCCCCCC) >> 2  | (v & 0x33333333) << 2;
        v = (v & 0xAAAAAAAA) >> 1  | (v & 0x55555555) << 1;
        return v;
    }
    static uint64 reverse(uint64 v)
    {
        v = (v & 0xFFFFFFFF00000000) >> 32 | (v & 0x00000000FFFFFFFF) << 32;
        v = (v & 0xFFFF0000FFFF0000) >> 16 | (v & 0x0000FFFF0000FFFF) << 16;
        v = (v & 0xFF00FF00FF00FF00) >> 8  | (v & 0x00FF00FF00FF00FF) << 8;
        v = (v & 0xF0F0F0F0F0F0F0F0) >> 4  | (v & 0x0F0F0F0F0F0F0F0F) << 4;
        v = (v & 0xCCCCCCCCCCCCCCCC) >> 2  | (v & 0x3333333333333333) << 2;
        v = (v & 0xAAAAAAAAAAAAAAAA) >> 1  | (v & 0x5555555555555555) << 1;
        return v;
    }
};

/** \cond */
namespace endian { namespace priv
{
    template<class Float>
    inline Float fromParts(const uint8* p)
    {
        union { Float f; uint8 bytes[sizeof(Float)]; } val;
        mt::for_<0, sizeof(Float)>([&](int i) { val.bytes[i] = p[i]; });
        return val.f;
    }
    
    template<class Float>
    inline Float fromPartsSwap(const uint8* p)
    {
        union { Float f; uint8 bytes[sizeof(Float)]; } val;
        mt::for_<0, sizeof(Float)>([&](int i) { val.bytes[i] = p[sizeof(Float)-1-i]; });
        return val.f;
    }
    
    template<class Float>
    inline void toParts(const Float f, uint8* p)
    {
        union { Float f; uint8 bytes[sizeof(Float)]; } val;
        val.f = f;
        mt::for_<0, sizeof(Float)>([&](int i) { p[i] = val.bytes[i]; });
    }
    
    template<class Float>
    inline void toPartsSwap(const Float f, uint8* p)
    {
        union { Float f; uint8 bytes[sizeof(Float)]; } val;
        val.f = f;
        mt::for_<0, sizeof(Float)>([&](int i) { p[i] = val.bytes[sizeof(Float)-1-i]; });
    }
} }
/** \endcond */

/// Bit util specific to endian type. Use through class BitOp.
template<int Endian> struct BitOpEndian {};

/// Specialization for little endian
template<>
struct BitOpEndian<static_cast<int>(Endian::little)> : BitOpCommon
{
    /// Get platform endian type
    static Endian platformEndian()                          { return Endian::little; }

    /// Convert integer from little endian to platform endian
    template<class Int>
    static Int littleToPlatform(const Int v)                { return v; }
    /// Convert integer from platform endian to little endian
    template<class Int>
    static Int platformToLittle(const Int v)                { return v; }

    /// Convert integer from big endian to platform endian
    template<class Int>
    static Int bigToPlatform(const Int v)                   { return swap(v); }
    /// Convert integer from platform endian to big endian
    template<class Int>
    static Int platformToBig(const Int v)                   { return swap(v); }
    
    /// \name Number serialization methods
    /// These methods can be used to serialize numbers in a platform-endian-agnostic manner (works on any machine).
    /// @{
    
    /// Convert an array of smaller number parts into a full number, where the first index holds the least significant part
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)                { return (T)BitOpCommon::fromPartsLittle<typename std::make_unsigned<T>::type>(p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)                { return endian::priv::fromParts<T>(p); }
    
    /// Convert an array of smaller number parts into a full number, where the first index holds the most significant part
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)                   { return (T)BitOpCommon::fromPartsBig<typename std::make_unsigned<T>::type>(p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)                   { return endian::priv::fromPartsSwap<T>(p); }
    
    /// Convert a full number into an array of smaller number parts, where the first index holds the least significant part
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static void toPartsLittle(const T v, uint8* p)          { BitOpCommon::toPartsLittle((typename std::make_unsigned<T>::type)v, p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static void toPartsLittle(const T v, uint8* p)          { endian::priv::toParts(v, p); }
    
    /// Convert a full number into an array of smaller number parts, where the first index holds the most significant part
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static void toPartsBig(const T v, uint8* p)             { BitOpCommon::toPartsBig((typename std::make_unsigned<T>::type)v, p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static void toPartsBig(const T v, uint8* p)             { endian::priv::toPartsSwap(v, p); }
    /// @}
};

/// Specialization for big endian
template<>
struct BitOpEndian<static_cast<int>(Endian::big)> : BitOpCommon
{
public:
    static Endian platformEndian()                          { return Endian::big; }

    template<class Int>
    static Int littleToPlatform(const Int v)                { return swap(v); }
    template<class Int>
    static Int platformToLittle(const Int v)                { return swap(v); }

    template<class Int>
    static Int bigToPlatform(const Int v)                   { return v; }
    template<class Int>
    static Int platformToBig(const Int v)                   { return v; }
    
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)                { return (T)BitOpCommon::fromPartsLittle<typename std::make_unsigned<T>::type>(p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)                { return endian::priv::fromPartsSwap<T>(p); }
    
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)                   { return (T)BitOpCommon::fromPartsBig<typename std::make_unsigned<T>::type>(p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)                   { return endian::priv::fromParts<T>(p); }
    
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static void toPartsLittle(const T v, uint8* p)          { BitOpCommon::toPartsLittle((typename std::make_unsigned<T>::type)v, p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static void toPartsLittle(const T v, uint8* p)          { endian::priv::toPartsSwap(v, p); }
    
    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type=0>
    static void toPartsBig(const T v, uint8* p)             { BitOpCommon::toPartsBig((typename std::make_unsigned<T>::type)v, p); }
    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type=0>
    static void toPartsBig(const T v, uint8* p)             { endian::priv::toParts(v, p); }
};

/// Provides methods for manipulating bits
typedef BitOpEndian<ENDIAN> BitOp;

};