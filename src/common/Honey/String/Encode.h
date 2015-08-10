// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/String/Stream.h"
#include "Honey/String/Bytes.h"
#include "Honey/Misc/MtMap.h"

namespace honey
{
/// Bytes to string encodings
namespace encode
{

/** \cond */
namespace priv
{
    mtkey(encode);
    mtkey(decode);
    mt_staticObj((std::map<Id, MtMap<   function<ostream& (ostream& os, const Bytes& val)>, encode,
                                        function<istream& (istream& os, Bytes& val)>, decode>>),
                                        exts,);
    
    struct Manip : honey::Manip<Manip> { Id encoding = "hex"_id; };
 
    extern const String hex_chars;
    extern const int8 hex_chars_rev[55];
    
    extern const String base64_chars;
    extern const int8 base64_chars_rev[80];
}
/** \endcond */

/// Register an encoding
bool reg(const Id& id,  const function<ostream& (ostream& os, const Bytes& val)>& encode,
                        const function<istream& (istream& os, Bytes& val)>& decode);
    
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
inline ostream& hex(ostream& os)            { priv::Manip::inst(os).encoding = "hex"_id; return os; }
/// Use hexadecimal decoding (high-nibble-first) when reading bytes from a string stream
inline istream& hex(istream& is)            { priv::Manip::inst(is).encoding = "hex"_id; return is; }


/// Check if character is in decimal charset (numeric)
inline bool isDec(Char c)                   { return c >= '0' && c <= '9'; }
/// Convert byte to decimal character
inline Char toDec(byte b)                   { return '0' + b; }
/// Convert decimal character to byte
inline byte fromDec(Char c)                 { return c - '0'; }

/// Convert bytes to string using decimal encoding (big-endian integer)
String dec_encode(const byte* data, int len);
/// Convert string to bytes using decimal decoding (big-endian integer)
Bytes dec_decode(const String& string);

/// Use decimal encoding (big-endian integer) when writing bytes to a string stream
inline ostream& dec(ostream& os)            { priv::Manip::inst(os).encoding = "dec"_id; return os; }
/// Use decimal decoding (big-endian integer) when reading bytes from a string stream
inline istream& dec(istream& is)            { priv::Manip::inst(is).encoding = "dec"_id; return is; }


/// Use UTF-8 encoding when writing bytes to a string stream
inline ostream& u8(ostream& os)             { priv::Manip::inst(os).encoding = "u8"_id; return os; }
/// Use UTF-8 decoding when reading bytes from a string stream
inline istream& u8(istream& is)             { priv::Manip::inst(is).encoding = "u8"_id; return is; }


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
inline ostream& base64(ostream& os)         { priv::Manip::inst(os).encoding = "base64"_id; return os; }
/// Use base64 decoding when reading bytes from a string stream
inline istream& base64(istream& is)         { priv::Manip::inst(is).encoding = "base64"_id; return is; }

} }
