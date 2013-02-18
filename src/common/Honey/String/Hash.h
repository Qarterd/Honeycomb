// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Optional.h"
#include "Honey/String/Bytes.h"

namespace honey
{

class String;

/// Data hashing functions.  Produce a small fingerprint from a larger data set.  Two data sets may 'collide', producing the same fingerprint.
namespace hash
{

/** \cond */
/// Constexpr version of MurmurHash3_x86_32
namespace priv { namespace murmur_constexpr
{
    constexpr uint32 rotLeft(uint32 v, int32 n)      { return (v << n) | (v >> (32-n)); }
    constexpr uint32 fMix_1(uint32 h)                { return h^(h >> 16); }
    constexpr uint32 fMix_0(uint32 h)                { return fMix_1((h^(h >> 13))*0xc2b2ae35); }
    constexpr uint32 fMix(uint32 h)                  { return fMix_0((h^(h >> 16))*0x85ebca6b); }
    constexpr uint32 c1                              = 0xcc9e2d51;
    constexpr uint32 c2                              = 0x1b873593;
    
    template<int Endian>
    constexpr uint32 block(const char* data, int i)                 { return data[i*4] | data[i*4+1] << 8 | data[i*4+2] << 16 | data[i*4+3] << 24; }
    template<>
    constexpr uint32 block<ENDIAN_BIG>(const char* data, int i)     { return data[i*4] << 24 | data[i*4+1] << 16 | data[i*4+2] << 8 | data[i*4+3]; }
    
    constexpr uint32 tail(const char* data, int len, int nblocks, uint32 h1)
    {
        return rotLeft((
                ((len&3) >= 3 ? data[nblocks*4+2] << 16 : 0) |
                ((len&3) >= 2 ? data[nblocks*4+1] << 8 : 0) |
                ((len&3) >= 1 ? data[nblocks*4] : 0))
                *c1, 15)*c2 ^ h1;
    }

    constexpr uint32 loop(const char* data, int len, int nblocks, int i, uint32 h1)
    {
        return i < nblocks ?    loop(data, len, nblocks, i+1, rotLeft(rotLeft(block<ENDIAN>(data, i)*c1, 15)*c2 ^ h1, 13)*5 + 0xe6546b64) :
                                fMix(tail(data, len, nblocks, h1) ^ len);
    }
} }
/** \endcond */

/// Quickly generate a small hash value. Each seed value produces a unique hash from the same data.
int fast(const byte* data, int len, int seed = 0);
/// fast() for strings
inline int fast(const String& str, int seed = 0)                            { return fast(reinterpret_cast<const byte*>(str.data()), str.length()*sizeof(Char), seed); }
/// fast() for UTF-8 strings
inline int fast_u8(const std::string& str, int seed = 0)                    { return fast(reinterpret_cast<const byte*>(str.data()), (int)str.length(), seed); }
/// fast() for UTF-8 strings
inline constexpr int fast_u8(const char* str, int len, int seed = 0)        { return priv::murmur_constexpr::loop(str, len, len / 4, 0, seed); }

/// 256-bit secure hash value
struct sval : ByteArray<32>
{
    typedef array<uint64,4> IntArray;
    
    using ByteArray::ByteArray;

    const IntArray& ints() const        { return reinterpret_cast<const IntArray&>(*this); }
    IntArray& ints()                    { return reinterpret_cast<IntArray&>(*this); }
};

/// Generate a large secure hash value
/**
  * \param  key     Generate a keyed HMAC that can be used to verify message authenticity.
  *                 Each key produces a unique hash from the same data.
  */
sval secure(const byte* data, int len, optional<const sval&> key = optnull);
/// secure() for strings
inline sval secure(const String& str, optional<const sval&> key = optnull)  { return secure(reinterpret_cast<const byte*>(str.data()), str.length()*sizeof(Char), key); }

/// Generate secure keys derived from a password
/**
  * \param  salt        randomly generated value to combat precomputed hash table attacks
  * \param  iterCount   number of PBKDF2 hash iterations, makes process computationally expensive to attack
  * \param  keyCount    number of returned keys
  */
vector<sval> secureKeys(const String& password, const Bytes& salt, int iterCount, int keyCount);

} }
