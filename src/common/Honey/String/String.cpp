// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/String.h"
#include "Honey/String/Stream.h"

namespace honey
{

String& String::insert(szt pos, const String& str, szt subpos, szt sublen)
{
    if (sublen == npos) sublen = str.length() - subpos;
    Super::insert(pos, str, subpos, sublen);
    return *this;
}

String& String::insert(szt pos, const std::string& str, szt subpos, szt sublen)
{
    if (sublen == npos) sublen = str.length() - subpos;
    Super::insert(begin() + pos, str.data() + subpos, str.data() + subpos + sublen);
    return *this;
}

String& String::insert(szt pos, const Char* str, szt subpos, szt sublen)
{
    assert(str);
    if (sublen == npos) Super::insert(pos, str + subpos);
    else                Super::insert(pos, str + subpos, sublen);
    return *this;
}

String& String::insert(szt pos, const char* str, szt subpos, szt sublen)
{
    assert(str);
    if (sublen == npos) sublen = strlen(str) - subpos;
    Super::insert(begin() + pos, str + subpos, str + subpos + sublen);
    return *this;
}

String& String::replace(szt pos, szt len, const String& str, szt subpos, szt sublen)
{
    if (sublen == npos) sublen = str.length() - subpos;
    Super::replace(pos, len, str, subpos, sublen);
    return *this;
}

template<class Char, class Comp>
static int compare_(const Char* str, szt n, szt pos, szt len,
                    const Char* str2, szt n2, szt pos2, szt len2,
                    Comp&& comp)
{
    if (len == String::npos) len = n - pos;
    if (len2 == String::npos) len2 = n2 - pos2;
    if (pos + len > n) len = n - pos;
    if (pos2 + len2 > n2) len2 = n2 - pos2;
    
    szt length = len < len2 ? len : len2;
    for (szt i = 0; i < length; ++i)
    {
        if (comp(str[pos+i], str2[pos2+i])) return -1;
        if (comp(str2[pos2+i], str[pos+i])) return 1;
    }
    return len != len2 ? (len < len2 ? -1 : 1) : 0;
}

int String::icompare(szt pos, szt len, const String& str, szt subpos, szt sublen) const
{
    return compare_(data(), length(), pos, len, str.data(), str.length(), subpos, sublen, [](Char a, Char b) { return std::tolower(a) < std::tolower(b); });
}

String String::toLower() const
{
    String str;
    std::transform(begin(), end(), std::back_inserter(str), (int (*)(int))std::tolower);
    return str;
}

String String::toUpper() const
{
    String str;
    std::transform(begin(), end(), std::back_inserter(str), (int (*)(int))std::toupper);
    return str;
}


/// Iterator class for tokenizing strings by a delimiter
class Tokenizer
{
public:
    typedef std::input_iterator_tag iterator_category;
    typedef String                  value_type;
    typedef sdt                     difference_type;
    typedef String*                 pointer;
    typedef String&                 reference;
    
    Tokenizer(const String& str, const String& delim, String::const_iterator it) :
        _str(&str),
        _delim(&delim),
        _pos(it - _str->begin()),
        _tokenCount(0)
    {
        assert(!_delim->empty());
        operator++();
    }

    Tokenizer& operator++()
    {
        if (_pos >= _str->length() || _pos == String::npos)
        {
            _pos = String::npos;
            _token.clear();
            return *this;
        }

        if (_tokenCount > 0) _pos += _delim->length();
        
        szt lastPos = _pos;
        _pos = _str->find(*_delim, _pos);
        if (_pos == String::npos) _pos = _str->length();
        
        _token = _str->substr(lastPos, _pos - lastPos);
        ++_tokenCount;

        return *this;
    }

    bool operator==(const Tokenizer& rhs) const     { return _pos == rhs._pos; }
    bool operator!=(const Tokenizer& rhs) const     { return !operator==(rhs); }

    const String& operator*() const                 { return _token; }

private:
    const String* _str;
    const String* _delim;
    
    szt _pos;
    String _token;
    szt _tokenCount;
};

String::List String::split(const String& delim, szt pos, szt count) const
{
    List ret;
    if (count == npos) count = length();
    std::copy(Tokenizer(*this, delim, begin()+pos), Tokenizer(*this, delim, begin()+pos+count), std::back_inserter(ret));
    return ret;
}


class Linker
{
public:
    typedef std::output_iterator_tag    iterator_category;
    typedef void                        value_type;
    typedef sdt                         difference_type;
    typedef void*                       pointer;
    typedef void                        reference;

    Linker(String& str, const String& delim)            : _str(&str), _delim(&delim), _token(0) {}

    Linker& operator=(const String& str)
    {
        if (_token > 0) _str->append(*_delim);
        _str->append(str);
        return *this;
    }

    Linker& operator++()                                { ++_token; return *this; }
    Linker& operator*()                                 { return *this; }

private:
    String* _str;
    const String* _delim;
    szt _token;
};

String String::join(const List& strings, const String& delim, szt start, szt end)
{
    auto itStart = start != npos ? strings.begin() + start : strings.begin();
    auto itEnd = end != npos ? strings.begin() + end : strings.end();

    String ret;
    std::copy(itStart, itEnd, Linker(ret, delim));
    return ret;
}

}
