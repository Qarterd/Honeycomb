// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Misc/BloomFilter.h"

namespace honey
{


    
/** \cond */
namespace bloom_filter { namespace priv
{
    //Chacha rand;
    //for (auto i : range(40)) { debug_print(sout() << "0x" << toBytes(rand.next()) << ", "); }
    const uint32 seeds[] =
    {
        0x3c4c9a94, 0x90613611, 0xa7a1cc07, 0x6f30f04d, 0x8a661bf9, 0xd75db7b2, 0x69604c69, 0xb253c947, 0xd2e373b2, 0xe7c98da4,
        0x8198391f, 0x88e9fa37, 0x6104fb19, 0xaf937c53, 0x9fcd60b4, 0xf1739c8d, 0x5486f17f, 0x0b16af9f, 0xb74d8b23, 0x69672728,
        0x45e140de, 0xb7593ff6, 0x1e3edca9, 0x094253fd, 0x292d6971, 0x2a5dd7ba, 0x09d5675f, 0xfd94c32c, 0x36b4c151, 0x6ae80e45,
        0x9614847a, 0x492e73c2, 0x95164f62, 0x4bd680a2, 0xecd0768f, 0x1d6b889b, 0x33650cfc, 0x60f7959b, 0xb1390020, 0xe1ead353,
    };
    const szt seedCount = sizeof(seeds) / sizeof(uint32);
} }
/** \endcond */
}