// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Bytes.h"
#include "Honey/String/Encode.h"

namespace honey
{

ostream& operator<<(ostream& os, const Bytes& val)
{
    switch (encode::priv::EncoderManip::inst(os).encoding)
    {
    case encode::priv::Encoding::hex:
        os << encode::hex_encode(val.data(), size(val));
        break;
    case encode::priv::Encoding::dec:
        os << encode::dec_encode(val.data(), size(val));
        break;
    case encode::priv::Encoding::u8:
        os << std::string(val.begin(), val.end());
        break;
    case encode::priv::Encoding::base64:
        os << encode::base64_encode(val.data(), size(val));
        break;
    case encode::priv::Encoding::base58:
        os << encode::base58_encode(val.data(), size(val));
        break;
    }
    
    return os;
}

istream& operator>>(istream& is, Bytes& val)
{
    switch (encode::priv::EncoderManip::inst(is).encoding)
    {
    case encode::priv::Encoding::hex:
        {
            String str;
            auto flags = is.flags(); //save flags
            is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
            while (encode::isHex(is.peek()))
            {
                char c;
                is >> c;
                str += c;
            }
            is.flags(flags); //restore flags
            val = encode::hex_decode(str);
            break;
        }
    case encode::priv::Encoding::dec:
        {
            String str;
            auto flags = is.flags(); //save flags
            is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
            while (encode::isDec(is.peek()))
            {
                char c;
                is >> c;
                str += c;
            }
            is.flags(flags); //restore flags
            val = encode::dec_decode(str);
            break;
        }
    case encode::priv::Encoding::u8:
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
            break;
        }
    case encode::priv::Encoding::base64:
        {
            String str;
            auto flags = is.flags(); //save flags
            is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
            while (encode::isBase64(is.peek()))
            {
                char c;
                is >> c;
                str += c;
            }
            is.flags(flags); //restore flags
            val = encode::base64_decode(str);
            break;
        }
    case encode::priv::Encoding::base58:
        {
            String str;
            auto flags = is.flags(); //save flags
            is >> std::ws >> std::noskipws; //skip initial whitespace then disable skipping
            while (encode::isBase58(is.peek()))
            {
                char c;
                is >> c;
                str += c;
            }
            is.flags(flags); //restore flags
            val = encode::base58_decode(str);
            break;
        }
    }
    
    return is;
}

}
