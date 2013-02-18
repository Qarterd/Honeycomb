// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Encode.h"
#include "Honey/Misc/Range.h"

namespace honey { namespace encode
{

/** \cond */
namespace priv
{
    const String hex_chars =
        "0123456789"
        "abcdef";

    const int8 hex_chars_rev[55] =
    {
        0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,
        -1,-1,-1,-1,-1,-1,-1,10,11,12,
        13,14,15,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,10,
        11,12,13,14,15
    };
    
    const String base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/=";

    const int8 base64_chars_rev[80] =
    {
        62,-1,-1,-1,63,52,53,54,55,56,
        57,58,59,60,61,-1,-1,-1,64,-1,
        -1,-1,0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,
        8 ,9 ,10,11,12,13,14,15,16,17,
        18,19,20,21,22,23,24,25,-1,-1,
        -1,-1,-1,-1,26,27,28,29,30,31,
        32,33,34,35,36,37,38,39,40,41,
        42,43,44,45,46,47,48,49,50,51
    };
    
    const String base58_chars =
        "123456789"
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "abcdefghijkmnopqrstuvwxyz";

    const int8 base58_chars_rev[74] =
    {
        0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,-1,
        -1,-1,-1,-1,-1,-1,9 ,10,11,12,
        13,14,15,16,-1,17,18,19,20,21,
        -1,22,23,24,25,26,27,28,29,30,
        31,32,-1,-1,-1,-1,-1,-1,33,34,
        35,36,37,38,39,40,41,42,43,-1,
        44,45,46,47,48,49,50,51,52,53,
        54,55,56,57
    };
}
/** \endcond */

String hex_encode(const byte* data, int len)
{
    String ret;
    ret.reserve(len*2);

    for (auto b: range(len))
    {
        ret += toHex(data[b] >> 4);
        ret += toHex(data[b] & 0xf);
    }
    return ret;
}

Bytes hex_decode(const String& string)
{
    Bytes ret;
    ret.reserve(string.length()/2);
    
    for (auto i: range(0, (string.length()/2)*2, 2))
    {
        if (!isHex(string[i]) || !isHex(string[i+1])) break;
        ret.push_back((fromHex(string[i]) << 4) | fromHex(string[i+1]));
    }
    return ret;
}

String base64_encode(const byte* data, int len)
{
    String ret;
    ret.reserve((len*4)/3 + 4); //round to full block
    int i = 0;
    byte chars_3[3], chars_4[4];
    
    for (auto b: range(len))
    {
        chars_3[i++] = data[b];
        if (i == 3)
        {
            chars_4[0] = (chars_3[0] & 0xfc) >> 2;
            chars_4[1] = ((chars_3[0] & 0x03) << 4) + ((chars_3[1] & 0xf0) >> 4);
            chars_4[2] = ((chars_3[1] & 0x0f) << 2) + ((chars_3[2] & 0xc0) >> 6);
            chars_4[3] = chars_3[2] & 0x3f;

            for (auto i: range(4)) ret += toBase64(chars_4[i]);
            i = 0;
        }
    }

    if (i)
    {
        for (auto j: range(i, 3)) chars_3[j] = '\0';

        chars_4[0] = (chars_3[0] & 0xfc) >> 2;
        chars_4[1] = ((chars_3[0] & 0x03) << 4) + ((chars_3[1] & 0xf0) >> 4);
        chars_4[2] = ((chars_3[1] & 0x0f) << 2) + ((chars_3[2] & 0xc0) >> 6);
        chars_4[3] = chars_3[2] & 0x3f;

        for (auto j: range(i+1)) ret += toBase64(chars_4[j]);
        while (i++ < 3) ret += '=';
    }
    return ret;
}

Bytes base64_decode(const String& string)
{
    int i = 0;
    byte chars_3[3], chars_4[4];
    Bytes ret;
    ret.reserve((3*string.length())/4);
    
    for (auto e: string)
    {
        if (!isBase64(e) || e == '=') break;
        chars_4[i++] = e;
        if (i == 4)
        {
            for(auto i: range(4)) chars_4[i] = fromBase64(chars_4[i]);

            chars_3[0] = (chars_4[0] << 2) + ((chars_4[1] & 0x30) >> 4);
            chars_3[1] = ((chars_4[1] & 0xf) << 4) + ((chars_4[2] & 0x3c) >> 2);
            chars_3[2] = ((chars_4[2] & 0x3) << 6) + chars_4[3];

            for (auto i: range(3)) ret.push_back(chars_3[i]);
            i = 0;
        }
    }

    if (i)
    {
        for(auto j: range(i, 4)) chars_4[j] = 0;
        for(auto j: range(4)) chars_4[j] = fromBase64(chars_4[j]);

        chars_3[0] = (chars_4[0] << 2) + ((chars_4[1] & 0x30) >> 4);
        chars_3[1] = ((chars_4[1] & 0xf) << 4) + ((chars_4[2] & 0x3c) >> 2);
        chars_3[2] = ((chars_4[2] & 0x3) << 6) + chars_4[3];

        for(auto j: range(i-1)) ret.push_back(chars_3[j]);
    }
    return ret;
}
 
String base58_encode(const byte* data, int len)
{
    const byte* cur = data, *end = data + len;
    
    //skip and count leading zeroes
    int zeroes;
    for (zeroes = 0; cur != end && *cur == 0; ++cur, ++zeroes);
    
    //convert big-endian base256 integer to base58 integer
    Bytes b58((end - cur) * 138 / 100 + 1); //length * log(256) / log(58), rounded up
    int high, j;
    for (high = size(b58); cur != end; ++cur, high = j+1)
    {
        //b58 = b58 * 256 + ch
        int carry;
        for (carry = *cur, j = size(b58)-1; j >= high || carry; --j)
        {
            assert(j >= 0);
            carry += 256 * b58[j];
            b58[j] = carry % 58;
            carry /= 58;
        }
    }
    
    //skip leading zeroes in base58 result
    auto it = b58.begin() + high;
    
    //translate the result into a string
    String ret;
    ret.reserve(zeroes + (b58.end() - it));
    ret.assign(zeroes, toBase58(0));
    while (it != b58.end()) ret += toBase58(*(it++));
    return ret;
}

Bytes base58_decode(const String& string)
{
    auto cur = string.begin(), end = string.end();
    
    //skip and count leading '1's
    int zeroes;
    for (zeroes = 0; cur != end && *cur == toBase58(0); ++cur, ++zeroes);
    
    //convert big-endian base58 integer to base256 integer
    Bytes b256((end - cur) * 733 / 1000 + 1); //length * log(58) / log(256), rounded up
    int high, j;
    for (high = size(b256); cur != end && isBase58(*cur); ++cur, high = j+1)
    {
        //b256 = b256 * 58 + ch
        int carry;
        for (carry = fromBase58(*cur), j = size(b256)-1; j >= high || carry; --j)
        {
            assert(j >= 0);
            carry += 58 * b256[j];
            b256[j] = carry % 256;
            carry /= 256;
        }
    }
    
    //skip leading zeroes in base256 result
    auto it = b256.begin() + high;
    
    //assemble the final bytes
    Bytes ret;
    ret.reserve(zeroes + (b256.end() - it));
    ret.assign(zeroes, 0);
    while (it != b256.end()) ret.push_back(*(it++));
    return ret;
}

} }
