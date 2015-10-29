// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Misc/BloomFilter.h"

namespace honey
{
/** \cond */
namespace bloom_filter { namespace priv
{
    //Chacha rand;
    //for (auto i : range(40)) { debug_print(sout() << "0x" << toBytes(rand.next()) << ", "); }
    const szt seeds[] =
    {
        0xdb4483562a5f36ff, 0x840395f33a66af21, 0xca05f26b983b1608, 0x417f61cdfa51ec50, 0xc4f71bdaa61c5f0a,
        0x65f9ad3f686bcbf0, 0x5d1f06f6263e1fad, 0xd870e5b3fe55788c, 0x363cce99f9622ebb, 0xace25fdd05a5494f,
        0x30e35db34e2c50ca, 0x9a16df8ff9bfcfcd, 0x2d1b1ee0a537b6ad, 0xdad1df07fa690423, 0x2d242813358715d6,
        0x9dad2673f9457363, 0x663c165b2fff434b, 0x63cea82928715856, 0x190f3e857fc968b6, 0xb13e8aa9e4ce9f00,
        0x31c367fc2cf7d88b, 0xe1857d120be7734d, 0xb8919e122e4a0500, 0x05a609f1a073deff, 0x6a885c7c03f6591f,
        0xa1d1a11603e43d1d, 0x669e257e57ab5125, 0xe92c0e2c250e9577, 0xa57d9b9627836fb7, 0x7d3d61cc58e6dbb1,
        0x6d91d2dffc3aa41b, 0x7eb6b9b082e29a40, 0x17562244bddb5f25, 0x8c97b82d0b35f465, 0xc96ba73c8e9cc097,
        0xa17339b6ac9cfdea, 0xf9de67190407b36d, 0x2874c33264c963d6, 0x4eebd83e6ad8fa4b, 0xec84478e2553ad76
    };
    const szt seedCount = sizeof(seeds) / sizeof(szt);
} }
/** \endcond */
}