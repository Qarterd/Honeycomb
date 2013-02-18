// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/String/Stream.h"
#include "Honey/String/Bytes.h"

namespace honey
{
/// Bytes to string encodings
namespace encode
{

/** \cond */
namespace priv
{
    enum class Encoding
    {
        hex,
        u8,
        base64,
        base58
    };
    
    struct EncoderManip : Manip<EncoderManip>
    {
        Encoding encoding = Encoding::hex;
    };
    
    extern const String hex_chars;
    extern const int8 hex_chars_rev[55];
    
    extern const String base64_chars;
    extern const int8 base64_chars_rev[80];
    
    extern const String base58_chars;
    extern const int8 base58_chars_rev[74];
}
/** \endcond */


/// Check if character is in hexadecimal charset (numeric and case-insensitive [a-f])
inline bool isHex(Char c)                   { return c >= '0' && c < '0' + sizeof(priv::hex_chars_rev) && priv::hex_chars_rev[c - '0'] != -1; }
/// Convert byte to hexadecimal character
inline Char toHex(byte b)                   { return priv::hex_chars[b]; }
/// Convert hexadecimal character to byte
inline byte fromHex(Char c)                 { return priv::hex_chars_rev[c - '0']; }

/// Convert bytes to string using hexadecimal encoding (high-nibble-first)
String hex_encode(const byte* data, int len);
/// Convert string to bytes using hexadecimal decoding (high-nibble-first)
Bytes hex_decode(const String& string);

/// Use hexadecimal encoding (high-nibble-first) when writing bytes to a string stream
inline ostream& hex(ostream& os)            { priv::EncoderManip::inst(os).encoding = priv::Encoding::hex; return os; }
/// Use hexadecimal decoding (high-nibble-first) when reading bytes from a string stream
inline istream& hex(istream& is)            { priv::EncoderManip::inst(is).encoding = priv::Encoding::hex; return is; }


/// Use UTF-8 encoding when writing bytes to a string stream
inline ostream& u8(ostream& os)             { priv::EncoderManip::inst(os).encoding = priv::Encoding::u8; return os; }
/// Use UTF-8 decoding when reading bytes from a string stream
inline istream& u8(istream& is)             { priv::EncoderManip::inst(is).encoding = priv::Encoding::u8; return is; }


/// Check if character is in base64 charset (alphanumeric and [+/=])
inline bool isBase64(Char c)                { return c >= '+' && c < '+' + sizeof(priv::base64_chars_rev) && priv::base64_chars_rev[c - '+'] != -1; }
/// Convert byte to base64 character
inline Char toBase64(byte b)                { return priv::base64_chars[b]; }
/// Convert base64 character to byte
inline byte fromBase64(Char c)              { return priv::base64_chars_rev[c - '+']; }

/// Convert bytes to string using base64 encoding
String base64_encode(const byte* data, int len);
/// Convert string to bytes using base64 decoding
Bytes base64_decode(const String& string);

/// Use base64 encoding when writing bytes to a string stream
inline ostream& base64(ostream& os)         { priv::EncoderManip::inst(os).encoding = priv::Encoding::base64; return os; }
/// Use base64 decoding when reading bytes from a string stream
inline istream& base64(istream& is)         { priv::EncoderManip::inst(is).encoding = priv::Encoding::base64; return is; }


/// Check if character is in base58 charset (alphanumeric except [0IOl])
inline bool isBase58(Char c)                { return c >= '1' && c < '1' + sizeof(priv::base58_chars_rev) && priv::base58_chars_rev[c - '1'] != -1; }
/// Convert byte to base58 character
inline Char toBase58(byte b)                { return priv::base58_chars[b]; }
/// Convert base58 character to byte
inline byte fromBase58(Char c)              { return priv::base58_chars_rev[c - '1']; }

/// Convert bytes to string using base58 encoding
String base58_encode(const byte* data, int len);
/// Convert string to bytes using base58 decoding
Bytes base58_decode(const String& string);

/// Use base58 encoding when writing bytes to a string stream
inline ostream& base58(ostream& os)         { priv::EncoderManip::inst(os).encoding = priv::Encoding::base58; return os; }
/// Use base58 decoding when reading bytes from a string stream
inline istream& base58(istream& is)         { priv::EncoderManip::inst(is).encoding = priv::Encoding::base58; return is; }

} }
