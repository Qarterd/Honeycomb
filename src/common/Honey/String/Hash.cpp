// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Hash.h"
#include "Honey/String/Stream.h"
#include "blake2.h"

namespace honey { namespace hash
{

/** \cond */
/// MurmurHash3_x86_32
namespace priv { namespace murmur
{
    template<int Endian>
    uint32 block(const uint32* p, szt i)                    { return p[i]; }
    template<>
    uint32 block<ENDIAN_BIG>(const uint32* p, szt i)        { return BitOp::swap(p[i]); }

    uint32 fMix(uint32 h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }

    uint32 hash(const void* key, szt len, uint32 seed)
    {
        const uint8* data = (const uint8*)key;
        const szt nblocks = len / 4;

        uint32 h1 = seed;

        uint32 c1 = 0xcc9e2d51;
        uint32 c2 = 0x1b873593;

        //----------
        // body

        const uint32* blocks = (const uint32*)data;

        for(szt i = 0; i < nblocks; ++i)
        {
            uint32 k1 = block<ENDIAN>(blocks,i);

            k1 *= c1;
            k1 = BitOp::rotLeft(k1,15);
            k1 *= c2;
    
            h1 ^= k1;
            h1 = BitOp::rotLeft(h1,13); 
            h1 = h1*5+0xe6546b64;
        }

        //----------
        // tail

        const uint8* tail = (const uint8*)(data + nblocks*4);

        uint32 k1 = 0;

        switch(len & 3)
        {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1; k1 = BitOp::rotLeft(k1,15); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= (uint32)len;

        h1 = fMix(h1);

        return h1;
    }
} }
/** \endcond */

int fast(const byte* data, szt len, int seed)
{
    return priv::murmur::hash(data, len, seed);
}

sval secure(const byte* data, szt len, optional<const sval&> key)
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
        blake2b_update(&S, data, len);
        blake2b_final(&S, res.data(), res.size());
  
        blake2b_init(&S, res.size());
        blake2b_update(&S, okeypad.data(), okeypad.size());
        blake2b_update(&S, res.data(), res.size());
        blake2b_final(&S, res.data(), res.size());
    }
    else
    {
        blake2b(res.data(), data, nullptr, res.size(), len, 0);
    }
    return res;
}

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
                u = secure(salt_k.data(), salt_k.size(), passkey);
                key = u;
                continue;
            }
            u = secure(u.data(), u.size(), passkey);
            for (auto i : range(key.ints().size())) key.ints()[i] ^= u.ints()[i];
        }
    }
    return res;
}

} }
