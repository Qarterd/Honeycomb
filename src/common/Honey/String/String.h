// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Debug.h"

namespace honey
{
    
/// \defgroup String    String methods
/// @{

/// Represents a single code unit (not code point) for class String
typedef char16_t Char;

/// Unicode UTF-16 string class, wrapper around std::u16string
/**
  * length() returns the number of UTF-16 code units, not code points (characters or visual symbols).
  * Some code points are 32-bit and thus are composed of 2 code units, called a surrogate pair or high / low surrogates.
  *
  * Modeling the string as an array of fixed-length UTF-16 code units rather than variable-length code points
  * is a trade-off for efficiency, as a large range of common characters can be represented by a single code unit.
  */
class String : public std::basic_string<Char>
{
    typedef std::basic_string<Char> Super;

public:
    /// List of strings
    typedef vector<String> List;
    
    using Super::basic_string;
    String() = default;
    /// Copy UTF-16 string
    String(const std::basic_string<Char>& str)                              : Super(str) {}
    /// Move UTF-16 string
    String(std::basic_string<Char>&& str)                                   : Super(move(str)) {}
    /// Convert from UTF-8 string
    String(const std::string& str)                                          : Super(str.begin(), str.end()) {}
    /// Convert from UTF-8 stringstream
    String(const ostream& os)                                               : String(static_cast<const ostringstream&>(os).str()) {}
    /// Convert from UTF-8 string, pointer must not be null
    String(const char* str, szt len = npos)                                 { assign(str, 0, len); }
    /// Convert from UTF-8 char repeated `n` times
    String(szt n, char c)                                                   { assign(n, c); }
    
    /// Forwards to assign()
    template<class T>
    String& operator=(T&& rhs)                                              { return assign(forward<T>(rhs)); }
    String& operator+=(const String& rhs)                                   { return append(rhs); }
    String& operator+=(Char rhs)                                            { return append(szt(1), rhs); }
    String& operator+=(char rhs)                                            { return append(szt(1), rhs); }
    
    String& clear()                                                         { Super::clear(); return *this; }

    /// @{
    /// Forwards to insert() at back
    template<class T>
    String& append(T&& str, szt subpos = 0, szt sublen = npos)              { return insert(length(), forward<T>(str), subpos, sublen); }
    String& append(szt n, Char c)                                           { return insert(length(), n, c); }
    String& append(szt n, char c)                                           { return insert(length(), n, c); }
    template<class InputIterator>
    String& append(InputIterator first, InputIterator last)                 { insert(begin() + length(), first, last); return *this; }
    /// @}

    /// @{
    /// Clears and forwards to append()
    template<class T>
    String& assign(T&& str, szt subpos = 0, szt sublen = npos)              { clear(); return append(forward<T>(str), subpos, sublen); }
    String& assign(szt n, Char c)                                           { clear(); return append(n, c); }
    String& assign(szt n, char c)                                           { clear(); return append(n, c); }
    template<class InputIterator>
    String& assign(InputIterator first, InputIterator last)                 { clear(); return append<InputIterator>(first, last); }
    /// @}
    
    String& insert(szt pos, const String& str, szt subpos = 0, szt sublen = npos);
    String& insert(szt pos, const Char* str, szt subpos = 0, szt sublen = npos);
    String& insert(szt pos, szt n, Char c)                                  { Super::insert(pos, n, c); return *this; }
    String& insert(szt pos, const char* str, szt subpos = 0, szt sublen = npos);
    String& insert(szt pos, szt n, char c)                                  { return insert(pos, n, static_cast<Char>(c)); }
    iterator insert(const_iterator p, Char c)                               { return Super::insert(p, c); }
    iterator insert(const_iterator p, szt n, Char c)                        { return Super::insert(p, n, c); }
    template<class InputIterator>
    iterator insert(const_iterator p, InputIterator first, InputIterator last)  { return Super::insert(p, first, last); }

    String& erase(szt pos = 0, szt len = npos)                              { Super::erase(pos, len); return *this; }
    iterator erase(const_iterator position)                                 { return Super::erase(position); }
    iterator erase(const_iterator first, const_iterator last)               { return Super::erase(first, last); }

    String& replace(szt pos, szt len, const String& str, szt subpos = 0, szt sublen = npos);
    String& replace(const_iterator i1, const_iterator i2, const String& str)                        { Super::replace(i1, i2, str); return *this; }
    template<class InputIterator>
    String& replace(const_iterator i1, const_iterator i2, InputIterator first, InputIterator last)  { Super::replace(i1, i2, first, last); return *this; }

    szt copy(Char* s, szt len, szt pos = 0) const                           { return Super::copy(s, len, pos); }
    szt copy(char* s, szt len, szt pos = 0) const                           { std::string str(begin() + pos, begin() + pos + len); return str.copy(s, str.length()); }

    String substr(szt pos = 0, szt len = npos) const                        { return Super::substr(pos, len); }

    /// Case-insensitive compare
    int icompare(const String& str) const                                   { return icompare(0, npos, str); }
    int icompare(   szt pos, szt len,
                    const String& str, szt subpos = 0, szt sublen = npos) const;

    /// Split a string into a list of separate substrings delimited by delim
    List split(const String& delim = String(1, ' '), szt pos = 0, szt count = npos) const;

    /// Join list into one string, separated by delim
    static String join(const List& strings, const String& delim = String(1, ' '), szt start = npos, szt end = npos);

    /// Convert string to lower case
    String toLower() const;
    /// Convert string to upper case
    String toUpper() const;
    /// Convert to UTF-8 string
    std::string u8() const                                                  { return std::string(begin(), end()); }
    
    /// Same as u8()
    operator std::string() const                                            { return u8(); }
 
    friend ostream& operator<<(ostream& os, const String& str)              { return os << str.u8(); }
    friend istream& operator>>(istream& is, String& str)                    { std::string str_; is >> str_; str = str_; return is; }
};

/// \name String methods
/// @{

/// Ensures that str points to a valid C-style string.  If str is null then the result is an empty C-string (ie. "").
inline const Char* c_str(const Char* str)                                   { return str ? str : u""; }
inline const char* c_str(const char* str)                                   { return str ? str : ""; }

/// Concatenate strings
inline String operator+(const String& lhs, const String& rhs)               { return std::operator+(lhs, rhs); }
inline bool operator==(const String& lhs, const String& rhs)                { return std::operator==(lhs, rhs); }
inline bool operator!=(const String& lhs, const String& rhs)                { return std::operator!=(lhs, rhs); }
inline bool operator< (const String& lhs, const String& rhs)                { return std::operator<(lhs, rhs); }
inline bool operator> (const String& lhs, const String& rhs)                { return std::operator>(lhs, rhs); }
inline bool operator<=(const String& lhs, const String& rhs)                { return std::operator<=(lhs, rhs); }
inline bool operator>=(const String& lhs, const String& rhs)                { return std::operator>=(lhs, rhs); }
/// @}

/// @}

}

/** \cond */
namespace std
{
/** \endcond */
    /// \ingroup String
    /// @{
    
    /// Output UTF-16 string, pointer must not be null.
    inline ostream& operator<<(ostream& os, honey::Char* str)               { assert(str); return os << std::string(str, str + std::char_traits<honey::Char>::length(str)); }
    inline ostream& operator<<(ostream& os, honey::Char val)                { honey::Char str[] = {val, 0}; return os << str; }
    /// @}
/** \cond */
}
/** \endcond */
