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
class BitOpCommon
{
public:
    /// Rotate integer bits cyclically to the left
    template<class T>
    static T rotLeft(const T v, const int32 n)
    {
        typedef typename std::make_unsigned<T>::type Unsigned;
        return (T)(((Unsigned)v << n) | ((Unsigned)v >> (Numeral<T>::sizeBits-n)));
    }

    /// Rotate integer bits cyclically to the right
    template<class T>
    static T rotRight(const T v, const int32 n)             { return rotLeft(v, Numeral<T>::sizeBits-n); }

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
    static uint32   high(const uint64 v)                    { return v >> 32; }
    static int32    high(const int64 v)                     { return (int32)high((uint64)v); }
    static uint16   high(const uint32 v)                    { return v >> 16; }
    static int16    high(const int32 v)                     { return (int16)high((uint32)v); }
    static uint8    high(const uint16 v)                    { return v >> 8; }
    static int8     high(const int16 v)                     { return (int8)high((uint16)v); }

    /// Retrieve low bytes from integer
    static uint32   low(const uint64 v)                     { return (uint32)v; }
    static int32    low(const int64 v)                      { return (int32)v; }
    static uint16   low(const uint32 v)                     { return (uint16)v; }
    static int16    low(const int32 v)                      { return (int16)v; }
    static uint8    low(const uint16 v)                     { return (uint8)v; }
    static int8     low(const int16 v)                      { return (int8)v; }

    /// Convert smaller integer parts into a full integer.
    static uint64 fromParts(const uint32 hi, const uint32 lo)
    {
        return  ((uint64)lo      )|
                ((uint64)hi << 32);
    }

    static uint32 fromParts(const uint16 hi, const uint16 lo)
    {
        return  ((uint32)lo      )|
                ((uint32)hi << 16);
    }

    /// \name Integer serialization methods
    /// These methods can be used to serialize integers in a platform-endian-agnostic manner (works on any machine).
    /// @{
    
    /// Convert an array of smaller integer parts into a full integer, where the first index holds the least significant part
    template<class T, typename std::enable_if<std::is_same<T, uint16>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)
    {
        return  ((uint16)p[0]      )|
                ((uint16)p[1] <<  8);
    }

    template<class T, typename std::enable_if<std::is_same<T, uint32>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)
    {
        return  ((uint32)p[0]      )|
                ((uint32)p[1] <<  8)|
                ((uint32)p[2] << 16)|
                ((uint32)p[3] << 24);
    }

    template<class T, typename std::enable_if<std::is_same<T, uint64>::value, int>::type=0>
    static T fromPartsLittle(const uint8* p)
    {
        return  ((uint64)p[0]      )|
                ((uint64)p[1] <<  8)|
                ((uint64)p[2] << 16)|
                ((uint64)p[3] << 24)|
                ((uint64)p[4] << 32)|
                ((uint64)p[5] << 40)|
                ((uint64)p[6] << 48)|
                ((uint64)p[7] << 56);
    }

    static uint64 fromPartsLittle(const uint32* p)
    {
        return  ((uint64)p[0]      )|
                ((uint64)p[1] << 32);
    }

    /// Convert an array of smaller integer parts into a full integer, where the first index holds the most significant part
    template<class T, typename std::enable_if<std::is_same<T, uint16>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)
    {
        return  ((uint16)p[0] <<  8)|
                ((uint16)p[1]      );
    }

    template<class T, typename std::enable_if<std::is_same<T, uint32>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)
    {
        return  ((uint32)p[0] << 24)|
                ((uint32)p[1] << 16)|
                ((uint32)p[2] <<  8)|
                ((uint32)p[3]      );
    }

    template<class T, typename std::enable_if<std::is_same<T, uint64>::value, int>::type=0>
    static T fromPartsBig(const uint8* p)
    {
        return  ((uint64)p[0] << 56)|
                ((uint64)p[1] << 48)|
                ((uint64)p[2] << 40)|
                ((uint64)p[3] << 32)|
                ((uint64)p[4] << 24)|
                ((uint64)p[5] << 16)|
                ((uint64)p[6] <<  8)|
                ((uint64)p[7]      );
    }

    static uint64 fromPartsBig(const uint32* p)
    {
        return  ((uint64)p[0] << 32)|
                ((uint64)p[1]      );
    }

    /// Convert a full integer into an array of smaller integer parts, where the first index holds the least significant part
    static void toPartsLittle(const uint16 v, uint8* p)
    {
        p[0] = (uint8)(v      );
        p[1] = (uint8)(v >>  8);
    }

    static void toPartsLittle(const uint32 v, uint8* p)
    {
        p[0] = (uint8)(v      );
        p[1] = (uint8)(v >>  8);
        p[2] = (uint8)(v >> 16);
        p[3] = (uint8)(v >> 24);
    }

    static void toPartsLittle(const uint64 v, uint8* p)
    {
        p[0] = (uint8)(v      );
        p[1] = (uint8)(v >>  8);
        p[2] = (uint8)(v >> 16);
        p[3] = (uint8)(v >> 24);
        p[4] = (uint8)(v >> 32);
        p[5] = (uint8)(v >> 40);
        p[6] = (uint8)(v >> 48);
        p[7] = (uint8)(v >> 56);
    }

    static void toPartsLittle(const uint64 v, uint32* p)
    {
        p[0] = (uint32)(v      );
        p[1] = (uint32)(v >> 32);
    }

    /// Convert a full integer into an array of smaller integer parts, where the first index holds the most significant part
    static void toPartsBig(const uint16 v, uint8* p)
    {
        p[0] = (uint8)(v >>  8);
        p[1] = (uint8)(v      );
    }

    static void toPartsBig(const uint32 v, uint8* p)
    {
        p[0] = (uint8)(v >> 24);
        p[1] = (uint8)(v >> 16);
        p[2] = (uint8)(v >>  8);
        p[3] = (uint8)(v      );
    }

    static void toPartsBig(const uint64 v, uint8* p)
    {
        p[0] = (uint8)(v >> 56);
        p[1] = (uint8)(v >> 48);
        p[2] = (uint8)(v >> 40);
        p[3] = (uint8)(v >> 32);
        p[4] = (uint8)(v >> 24);
        p[5] = (uint8)(v >> 16);
        p[6] = (uint8)(v >>  8);
        p[7] = (uint8)(v      );
    }

    static void toPartsBig(const uint64 v, uint32* p)
    {
        p[0] = (uint32)(v >> 32);
        p[1] = (uint32)(v      );
    }
    /// @}
};

/// Bit util specific to endian type. Use through class BitOp.
template<int Endian> class BitOpEndian {};

/// Specialization for little endian
template<>
class BitOpEndian<static_cast<int>(Endian::little)>
{
public:
    /// Get platform endian type
    static Endian platformEndian()                          { return Endian::little; }

    /// Convert from little endian to platform endian
    template<class T>
    static T littleToPlatform(const T little)               { return little; }
    /// Convert from platform endian to little endian
    template<class T>
    static T platformToLittle(const T platform)             { return platform; }

    /// Convert from big endian to platform endian
    template<class T>
    static T bigToPlatform(const T big)                     { return BitOpCommon::swap(big); }
    /// Convert from platform endian to big endian
    template<class T>
    static T platformToBig(const T platform)                { return BitOpCommon::swap(platform); }
};

/// Specialization for big endian
template<>
class BitOpEndian<static_cast<int>(Endian::big)>
{
public:
    static Endian platformEndian()                          { return Endian::big; }

    template<class T>
    static T littleToPlatform(const T little)               { return BitOpCommon::swap(little); }
    template<class T>
    static T platformToLittle(const T platform)             { return BitOpCommon::swap(platform); }

    template<class T>
    static T bigToPlatform(const T big)                     { return big; }
    template<class T>
    static T platformToBig(const T platform)                { return platform; }
};


/// Provides methods for manipulating bits
class BitOp : public BitOpCommon, public BitOpEndian<ENDIAN> {};


};
