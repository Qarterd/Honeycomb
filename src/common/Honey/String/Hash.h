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
/// Compile-time version of MurmurHash3_x64_128
namespace priv { namespace murmur_constexpr
{
    constexpr uint64 rotLeft(uint64 v, uint64 n)     { return (v << n) | (v >> (64-n)); }
    constexpr uint64 block(const char* data, szt i)
    {
        return  ((uint64)data[i*8]) |
                ((uint64)data[i*8+1]) << 8 |
                ((uint64)data[i*8+2]) << 16 |
                ((uint64)data[i*8+3]) << 24 |
                ((uint64)data[i*8+4]) << 32 |
                ((uint64)data[i*8+5]) << 40 |
                ((uint64)data[i*8+6]) << 48 |
                ((uint64)data[i*8+7]) << 56;
    }
    
    constexpr uint64 fMix_1(uint64 k)                { return k^(k >> 33); }
    constexpr uint64 fMix_0(uint64 k)                { return fMix_1((k^(k >> 33))*0xc4ceb9fe1a85ec53); }
    constexpr uint64 fMix(uint64 k)                  { return fMix_0((k^(k >> 33))*0xff51afd7ed558ccd); }
    
    constexpr uint64 fin(uint64 h1, uint64 h2)
    {
        return fMix(h1+h2) + fMix(h1+h2+h2);
    }
    
    constexpr uint64 c1                              = 0x87c37b91114253d5;
    constexpr uint64 c2                              = 0x4cf5ad432745937f;
    
    constexpr uint64 tail_h1(const char* tail, szt len, uint64 h1)
    {
        return rotLeft((
                ((len&15) >= 8 ? ((uint64)tail[7]) << 56 : 0) |
                ((len&15) >= 7 ? ((uint64)tail[6]) << 48 : 0) |
                ((len&15) >= 6 ? ((uint64)tail[5]) << 40 : 0) |
                ((len&15) >= 5 ? ((uint64)tail[4]) << 32 : 0) |
                ((len&15) >= 4 ? ((uint64)tail[3]) << 24 : 0) |
                ((len&15) >= 3 ? ((uint64)tail[2]) << 16 : 0) |
                ((len&15) >= 2 ? ((uint64)tail[1]) << 8 : 0) |
                ((len&15) >= 1 ? ((uint64)tail[0]) : 0))
                *c1, 31)*c2 ^ h1;
    }
    constexpr uint64 tail_h2(const char* tail, szt len, uint64 h2)
    {
        return rotLeft((
                ((len&15) >= 15 ? ((uint64)tail[14]) << 48 : 0) |
                ((len&15) >= 14 ? ((uint64)tail[13]) << 40 : 0) |
                ((len&15) >= 13 ? ((uint64)tail[12]) << 32 : 0) |
                ((len&15) >= 12 ? ((uint64)tail[11]) << 24 : 0) |
                ((len&15) >= 11 ? ((uint64)tail[10]) << 16 : 0) |
                ((len&15) >= 10 ? ((uint64)tail[9]) << 8 : 0) |
                ((len&15) >= 9 ? ((uint64)tail[8]) : 0))
                *c2, 33)*c1 ^ h2;
    }
    
    constexpr uint64 loop_h1(const char* data, szt i, uint64 h1, uint64 h2)
    {
        return (rotLeft(rotLeft(block(data, i*2+0)*c1, 31)*c2 ^ h1, 27)+h2)*5 + 0x52dce729;
    }
    constexpr uint64 loop_h2(const char* data, szt i, uint64 h1, uint64 h2)
    {
        return (rotLeft(rotLeft(block(data, i*2+1)*c2, 33)*c1 ^ h1, 31)+loop_h1(data,i,h1,h2))*5 + 0x38495ab5;
    }
    constexpr uint64 loop(const char* data, szt len, szt nblocks, szt i, uint64 h1, uint64 h2)
    {
        return i < nblocks ?    loop(data, len, nblocks, i+1, loop_h1(data,i,h1,h2), loop_h2(data,i,h1,h2)) :
                                fin(tail_h1(data+nblocks*16,len,h1) ^ (uint64)len, tail_h2(data+nblocks*16,len,h2) ^ (uint64)len);
    }
} }
/** \endcond */

/// Quickly generate a small hash value. Each seed value produces a unique hash from the same data.
szt fast(ByteBufConst bs, szt seed = 0);
/// fast() for UTF-8 strings
inline szt fast(const char* str, szt seed = 0)                              { return fast(ByteBufConst(reinterpret_cast<const byte*>(str), strlen(str)), seed); }
/// fast() for UTF-8 strings
inline szt fast(const std::string& str, szt seed = 0)                       { return fast(ByteBufConst(reinterpret_cast<const byte*>(str.data()), str.length()), seed); }
/// fast() for strings, converted to UTF-8 before hashing
inline szt fast(const String& str, szt seed = 0);
/// Compile-time version of fast() for UTF-8 strings
inline constexpr szt fast_(const char* str, szt len, szt seed = 0)          { return priv::murmur_constexpr::loop(str, len, len / 16, 0, uint64(seed), uint64(seed)); }

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
  * \param  bs
  * \param  key     Generate a keyed HMAC that can be used to verify message authenticity.
  *                 Each key produces a unique hash from the same data.
  */
sval secure(ByteBufConst bs, optional<const sval&> key = optnull);
/// secure() for UTF-8 strings
inline sval secure(const char* str, optional<const sval&> key = optnull)        { return secure(ByteBufConst(reinterpret_cast<const byte*>(str), strlen(str)), key); }
/// secure() for UTF-8 strings
inline sval secure(const std::string& str, optional<const sval&> key = optnull) { return secure(ByteBufConst(reinterpret_cast<const byte*>(str.data()), str.length()), key); }
/// secure() for strings, converted to UTF-8 before hashing
sval secure(const String& str, optional<const sval&> key = optnull);

/// Generate secure keys derived from a password
/**
  * \param  password
  * \param  salt        randomly generated value to combat precomputed hash table attacks
  * \param  iterCount   number of PBKDF2 hash iterations, makes process computationally expensive to attack
  * \param  keyCount    number of returned keys
  */
vector<sval> secureKeys(const String& password, const Bytes& salt, int iterCount, int keyCount);

} }
