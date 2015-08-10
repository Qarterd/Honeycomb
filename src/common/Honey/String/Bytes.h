// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/BitOp.h"
#include "Honey/Misc/Range.h"

namespace honey
{

/// An unsigned 8-bit integer
typedef uint8 byte;

/// Construct byte from integer literal (eg. 128_b)
constexpr byte operator"" _b(unsigned long long int i)  { return static_cast<byte>(i); }
/// Construct byte from character literal (eg. 'x'_b)
constexpr byte operator"" _b(char c)                    { return static_cast<byte>(c); }

template<int N> struct ByteArray;
template<class Block, class Alloc> class BitSet_;
    
/// String of bytes
class Bytes : public vector<byte>
{
public:
    using vector::vector;
    
    Bytes() = default;
    template<int N>
    Bytes(const ByteArray<N>& bs)                       : vector(bs.begin(), bs.end()) {}
    
    /// Construct from big-endian bits (the first index contains the MSB)
    template<class Block, class Alloc>
    Bytes(const BitSet_<Block,Alloc>& bs)
    {
        resize(bs.size()/8 + (bs.size()%8 != 0));
        int i = 0;
        for (auto& b: *this) for (auto j: range(8)) if (i < bs.size()) b |= bs.test(i++) << (7-j);
    }
    
    /// Write bytes to string stream using current encoding
    friend ostream& operator<<(ostream& os, const Bytes& val);
    /// Read bytes from string stream using current decoding
    friend istream& operator>>(istream& is, Bytes& val);
};

/// Construct bytes from string literal (eg. "something"_b)
inline Bytes operator"" _b(const char* str, size_t len)     { return Bytes(str, str+len); }

/// Convert integral value to bytes
template<class Int>
typename std::enable_if<std::is_integral<Int>::value, Bytes>::type
    toBytes(Int val, Endian order = Endian::big)
{
    typedef typename std::make_unsigned<Int>::type Unsigned;
    Bytes bs(sizeof(Int));
    switch (order)
    {
    case Endian::little:
        BitOp::toPartsLittle(static_cast<Unsigned>(val), bs.data());
        break;
    case Endian::big:
        BitOp::toPartsBig(static_cast<Unsigned>(val), bs.data());
        break;
    }
    return bs;
}

/// Convert bytes to integral value
template<class Int>
typename std::enable_if<std::is_integral<Int>::value, Int>::type
    fromBytes(const Bytes& bs, Endian order = Endian::big)
{
    typedef typename std::make_unsigned<Int>::type Unsigned;
    switch (order)
    {
    case Endian::little:
        return static_cast<Int>(BitOp::fromPartsLittle<Unsigned>(bs.data()));
    case Endian::big:
        return static_cast<Int>(BitOp::fromPartsBig<Unsigned>(bs.data()));
    }
}


/// Fixed array of N bytes
template<int N>
struct ByteArray : array<byte, N>
{
    typedef array<byte, N> Super;
    
    ByteArray() = default;
    /// Construct from list of byte values
    template<class... Bytes>
    ByteArray(byte b, Bytes&&... bs)    : Super{b, forward<Bytes>(bs)...} {}
    ByteArray(const Bytes& bs)          { assert(bs.size() == this->size()); std::copy(bs.begin(), bs.end(), this->begin()); }
    
    /// Write byte array to string stream using current encoding
    friend ostream& operator<<(ostream& os, const ByteArray& val)   { os << Bytes(val); return os; }
    /// Read byte array from string stream using current decoding
    friend istream& operator>>(istream& is, ByteArray& val)         { Bytes bs; is >> bs; val = bs; return is; }
};

}
