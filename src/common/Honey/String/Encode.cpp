// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

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
    
    const String base32_chars =
        "abcdefghijklmnopqrstuvwxyz"
        "234567=";

    const int8 base32_chars_rev[73] =
    {
        26,27,28,29,30,31,-1,-1,-1,-1,
        -1,32,-1,-1,-1,0 ,1 ,2 ,3 ,4 ,
        5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,
        25,-1,-1,-1,-1,-1,-1,0 ,1 ,2 ,
        3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,
        13,14,15,16,17,18,19,20,21,22,
        23,24,25
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
}
/** \endcond */

bool reg(const Id& id,  const function<ostream& (ostream& os, const Bytes& val)>& encode,
                        const function<istream& (istream& os, Bytes& val)>& decode)
{
    assert(!priv::exts().count(id));
    priv::exts().insert(make_pair(id, mtmap(priv::encode() = encode, priv::decode() = decode)));
    return true;
}

static auto __hex = reg("hex"_id,
    [](ostream& os, const Bytes& val) -> ostream& { return os << hex_encode(val); },
    [](istream& is, Bytes& val) -> istream&
    {
        String str;
        auto flags = is.flags(); //save flags
        is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
        while (isHex(is.peek()))
        {
            char c;
            is >> c;
            str += c;
        }
        is.flags(flags); //restore flags
        val = hex_decode(str);
        return is;
    });
    
String hex_encode(ByteBufConst bs)
{
    String ret;
    ret.reserve(bs.size()*2);

    for (auto b: bs)
    {
        ret += toHex(b >> 4);
        ret += toHex(b & 0xf);
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

static auto __dec = reg("dec"_id,
    [](ostream& os, const Bytes& val) -> ostream& { return os << dec_encode(val); },
    [](istream& is, Bytes& val) -> istream&
    {
        String str;
        auto flags = is.flags(); //save flags
        is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
        while (isDec(is.peek()))
        {
            char c;
            is >> c;
            str += c;
        }
        is.flags(flags); //restore flags
        val = dec_decode(str);
        return is;
    });
    
String dec_encode(ByteBufConst bs)
{
    auto cur = bs.begin(), end = bs.end();
    
    //skip and count leading zeroes
    szt zeroes;
    for (zeroes = 0; cur != end && *cur == 0; ++cur, ++zeroes);
    
    //convert big-endian base256 integer to base10 integer
    Bytes b10((end - cur) * 241 / 100 + 1); //length * log(256) / log(10), rounded up
    sdt high, j;
    for (high = b10.size(); cur != end; ++cur, high = j+1)
    {
        //b10 = b10 * 256 + ch
        int carry;
        for (carry = *cur, j = b10.size()-1; j >= high || carry; --j)
        {
            assert(j >= 0);
            carry += 256 * b10[j];
            b10[j] = carry % 10;
            carry /= 10;
        }
    }
    
    //skip leading zeroes in base10 result
    auto it = b10.begin() + high;
    
    //translate the result into a string
    String ret;
    ret.reserve(zeroes + (b10.end() - it));
    ret.assign(zeroes, toDec(0));
    while (it != b10.end()) ret += toDec(*(it++));
    return ret;
}

Bytes dec_decode(const String& string)
{
    auto cur = string.begin(), end = string.end();
    
    //skip and count leading '0's
    szt zeroes;
    for (zeroes = 0; cur != end && *cur == toDec(0); ++cur, ++zeroes);
    
    //convert big-endian base10 integer to base256 integer
    Bytes b256((end - cur) * 416 / 1000 + 1); //length * log(10) / log(256), rounded up
    sdt high, j;
    for (high = b256.size(); cur != end && isDec(*cur); ++cur, high = j+1)
    {
        //b256 = b256 * 10 + ch
        int carry;
        for (carry = fromDec(*cur), j = b256.size()-1; j >= high || carry; --j)
        {
            assert(j >= 0);
            carry += 10 * b256[j];
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
    
static auto __u8 = reg("u8"_id,
    [](ostream& os, const Bytes& val) -> ostream& { return os << std::string(val.begin(), val.end()); },
    [](istream& is, Bytes& val) -> istream&
    {
        //all chars are valid, read until end
        String str;
        auto flags = is.flags(); //save flags
        is >> std::noskipws; //disable whitespace skipping
        while (is.peek() != std::stringstream::traits_type::eof())
        {
            char c;
            is >> c;
            str += c;
        }
        is.flags(flags); //restore flags
        val = Bytes(str.begin(), str.end());
        return is;
    });
    
static auto __base32 = reg("base32"_id,
    [](ostream& os, const Bytes& val) -> ostream& { return os << base32_encode(val); },
    [](istream& is, Bytes& val) -> istream&
    {
        String str;
        auto flags = is.flags(); //save flags
        is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
        while (isBase32(is.peek()))
        {
            char c;
            is >> c;
            str += c;
        }
        is.flags(flags); //restore flags
        val = base32_decode(str);
        return is;
    });
    
String base32_encode(ByteBufConst bs)
{
    auto cur = bs.begin(), end = bs.end();
    
    String ret;
    ret.reserve((bs.size()+4)/5*8); //round to full block
    
    int mode=0, left=0;
    while (cur < end)
    {
        auto enc = *(cur++);
        switch (mode)
        {
        case 0: // we have no bits
            ret += toBase32(enc >> 3);
            left = (enc & 7) << 2;
            mode = 1;
            break;

        case 1: // we have three bits
            ret += toBase32(left | (enc >> 6));
            ret += toBase32((enc >> 1) & 31);
            left = (enc & 1) << 4;
            mode = 2;
            break;

        case 2: // we have one bit
            ret += toBase32(left | (enc >> 4));
            left = (enc & 15) << 1;
            mode = 3;
            break;

        case 3: // we have four bits
            ret += toBase32(left | (enc >> 7));
            ret += toBase32((enc >> 2) & 31);
            left = (enc & 3) << 3;
            mode = 4;
            break;

        case 4: // we have two bits
            ret += toBase32(left | (enc >> 5));
            ret += toBase32(enc & 31);
            mode = 0;
        }
    }

    static const int padding[5] = {0, 6, 4, 3, 1};
    if (mode)
    {
        ret += toBase32(left);
        for (auto i: range(padding[mode])) { mt_unused(i); ret += '='; }
    }

    return ret;
}

Bytes base32_decode(const String& string)
{
    Bytes ret;
    ret.reserve((5*string.length())/8);

    int mode = 0, left = 0;
    for (auto e: string)
    {
        if (!isBase32(e) || e == '=') break;
        auto dec = fromBase32(e);
        switch (mode)
        {
        case 0: // we have no bits and get 5
            left = dec;
            mode = 1;
            break;

        case 1: // we have 5 bits and keep 2
            ret.push_back((left<<3) | (dec>>2));
            left = dec & 3;
            mode = 2;
            break;

        case 2: // we have 2 bits and keep 7
            left = left << 5 | dec;
            mode = 3;
            break;

        case 3: // we have 7 bits and keep 4
            ret.push_back((left<<1) | (dec>>4));
            left = dec & 15;
            mode = 4;
            break;

        case 4: // we have 4 bits, and keep 1
            ret.push_back((left<<4) | (dec>>1));
            left = dec & 1;
            mode = 5;
            break;

        case 5: // we have 1 bit, and keep 6
            left = left << 5 | dec;
            mode = 6;
            break;

        case 6: // we have 6 bits, and keep 3
            ret.push_back((left<<2) | (dec>>3));
            left = dec & 7;
            mode = 7;
            break;

        case 7: // we have 3 bits, and keep 0
            ret.push_back((left<<5) | dec);
            mode = 0;
            break;
        }
    }

    return ret;
}

static auto __base64 = reg("base64"_id,
    [](ostream& os, const Bytes& val) -> ostream& { return os << base64_encode(val); },
    [](istream& is, Bytes& val) -> istream&
    {
        String str;
        auto flags = is.flags(); //save flags
        is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
        while (isBase64(is.peek()))
        {
            char c;
            is >> c;
            str += c;
        }
        is.flags(flags); //restore flags
        val = base64_decode(str);
        return is;
    });
    
String base64_encode(ByteBufConst bs)
{
    String ret;
    ret.reserve((bs.size()*4)/3 + 4); //round to full block
    szt i = 0;
    byte chars_3[3], chars_4[4];
    
    for (auto b: bs)
    {
        chars_3[i++] = b;
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
    szt i = 0;
    byte chars_3[3], chars_4[4];
    Bytes ret;
    ret.reserve((3*string.length())/4);
    
    for (auto e: string)
    {
        if (!isBase64(e) || e == '=') break;
        chars_4[i++] = e;
        if (i == 4)
        { 
            for (auto i: range(4)) chars_4[i] = fromBase64(chars_4[i]);

            chars_3[0] = (chars_4[0] << 2) + ((chars_4[1] & 0x30) >> 4);
            chars_3[1] = ((chars_4[1] & 0xf) << 4) + ((chars_4[2] & 0x3c) >> 2);
            chars_3[2] = ((chars_4[2] & 0x3) << 6) + chars_4[3];

            for (auto i: range(3)) ret.push_back(chars_3[i]);
            i = 0;
        }
    }

    if (i)
    {
        for (auto j: range(i, 4)) chars_4[j] = 0;
        for (auto j: range(4)) chars_4[j] = fromBase64(chars_4[j]);

        chars_3[0] = (chars_4[0] << 2) + ((chars_4[1] & 0x30) >> 4);
        chars_3[1] = ((chars_4[1] & 0xf) << 4) + ((chars_4[2] & 0x3c) >> 2);
        chars_3[2] = ((chars_4[2] & 0x3) << 6) + chars_4[3];

        for (auto j: range(i-1)) ret.push_back(chars_3[j]);
    }
    return ret;
}
    
} }
