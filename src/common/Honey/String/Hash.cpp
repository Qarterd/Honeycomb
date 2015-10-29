// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/String/Hash.h"
#include "Honey/String/Stream.h"
#include "blake2.h"

namespace honey { namespace hash
{

/** \cond */
/// MurmurHash3_x64_128
namespace priv { namespace murmur
{
    template<int Endian>
    uint64 block(const uint64* p, szt i)                    { return p[i]; }
    template<>
    uint64 block<ENDIAN_BIG>(const uint64* p, szt i)        { return BitOp::swap(p[i]); }

    uint64 fMix(uint64 k)
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccd;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53;
        k ^= k >> 33;
        return k;
    }

    tuple<uint64, uint64> hash(const void* key, szt len, uint64 seed)
    {
        const uint8* data = (const uint8*)key;
        const szt nblocks = len / 16;

        uint64 h1 = seed;
        uint64 h2 = seed;
        
        uint64 c1 = 0x87c37b91114253d5;
        uint64 c2 = 0x4cf5ad432745937f;

        //----------
        // body

        const uint64* blocks = (const uint64*)data;

        for(szt i = 0; i < nblocks; ++i)
        {
            uint64 k1 = block<ENDIAN>(blocks,i*2+0);
            uint64 k2 = block<ENDIAN>(blocks,i*2+1);

            k1 *= c1; k1  = BitOp::rotLeft(k1,31); k1 *= c2; h1 ^= k1;
            h1 = BitOp::rotLeft(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

            k2 *= c2; k2  = BitOp::rotLeft(k2,33); k2 *= c1; h2 ^= k2;
            h2 = BitOp::rotLeft(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
        }

        //----------
        // tail

        const uint8* tail = (const uint8*)(data + nblocks*16);

        uint64 k1 = 0;
        uint64 k2 = 0;
        
        switch(len & 15)
        {
        case 15: k2 ^= ((uint64)tail[14]) << 48;
        case 14: k2 ^= ((uint64)tail[13]) << 40;
        case 13: k2 ^= ((uint64)tail[12]) << 32;
        case 12: k2 ^= ((uint64)tail[11]) << 24;
        case 11: k2 ^= ((uint64)tail[10]) << 16;
        case 10: k2 ^= ((uint64)tail[ 9]) << 8;
        case  9: k2 ^= ((uint64)tail[ 8]) << 0;
            k2 *= c2; k2 = BitOp::rotLeft(k2,33); k2 *= c1; h2 ^= k2;

        case  8: k1 ^= ((uint64)tail[ 7]) << 56;
        case  7: k1 ^= ((uint64)tail[ 6]) << 48;
        case  6: k1 ^= ((uint64)tail[ 5]) << 40;
        case  5: k1 ^= ((uint64)tail[ 4]) << 32;
        case  4: k1 ^= ((uint64)tail[ 3]) << 24;
        case  3: k1 ^= ((uint64)tail[ 2]) << 16;
        case  2: k1 ^= ((uint64)tail[ 1]) << 8;
        case  1: k1 ^= ((uint64)tail[ 0]) << 0;
            k1 *= c1; k1 = BitOp::rotLeft(k1,31); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= (uint64)len;  h2 ^= (uint64)len;
        h1 += h2;           h2 += h1;
        h1 = fMix(h1);      h2 = fMix(h2);
        h1 += h2;           h2 += h1;
        return make_tuple(h1, h2);
    }
} }
/** \endcond */

szt fast(ByteBufConst bs, szt seed)
{
    return get<0>(priv::murmur::hash(bs.data(), bs.size(), uint64(seed)));
}

szt fast(const String& str, szt seed)   { return fast(str.u8(), seed); }

sval secure(ByteBufConst bs, optional<const sval&> key)
{
    sval res;
    if (key)
    {
        //HMAC(K,m) = H(K ^ opad || H(K ^ ipad || m))
        sval okeypad, ikeypad;
        okeypad.ints().fill(0x5c5c5c5c5c5c5c5cU);
        ikeypad.ints().fill(0x3636363636363636U);
        for (auto i : range(key->ints().size()))
        {
            okeypad.ints()[i] ^= key->ints()[i];
            ikeypad.ints()[i] ^= key->ints()[i];
        }
        blake2b_state S;
        blake2b_init(&S, res.size());
        blake2b_update(&S, ikeypad.data(), ikeypad.size());
        blake2b_update(&S, bs.data(), bs.size());
        blake2b_final(&S, res.data(), res.size());
  
        blake2b_init(&S, res.size());
        blake2b_update(&S, okeypad.data(), okeypad.size());
        blake2b_update(&S, res.data(), res.size());
        blake2b_final(&S, res.data(), res.size());
    }
    else
    {
        blake2b(res.data(), bs.data(), nullptr, res.size(), bs.size(), 0);
    }
    return res;
}

sval secure(const String& str, optional<const sval&> key)   { return secure(str.u8(), key); }

vector<sval> secureKeys(const String& password, const Bytes& salt, int iterCount, int keyCount)
{
    vector<sval> res(keyCount);
    Bytes salt_k(salt.size() + sizeof(uint32));
    std::copy(salt.begin(), salt.end(), salt_k.begin());
    auto passkey = secure(password);
    for (auto k: range(keyCount))
    {
        sval& key = res[k];
        sval u;
        for (auto iter: range(iterCount))
        {
            if (!iter)
            {
                BitOp::toPartsBig(static_cast<uint32>(k)+1, salt_k.data()+salt.size());
                u = secure(salt_k, passkey);
                key = u;
                continue;
            }
            u = secure(u, passkey);
            for (auto i : range(key.ints().size())) key.ints()[i] ^= u.ints()[i];
        }
    }
    return res;
}

} }
