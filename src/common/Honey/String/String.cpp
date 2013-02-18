// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/String.h"
#include "Honey/String/Stream.h"

namespace honey
{

String& String::insert(int pos, const String& str, int subpos, int sublen)
{
    if (sublen == npos) sublen = str.length() - subpos;
    Super::insert(pos, str, subpos, sublen);
    return *this;
}

String& String::insert(int pos, const Char* str, int subpos, int sublen)
{
    assert(str);
    if (sublen == npos) Super::insert(pos, str + subpos);
    else                Super::insert(pos, str + subpos, sublen);
    return *this;
}

String& String::insert(int pos, const char* str, int subpos, int sublen)
{
    assert(str);
    if (sublen == npos) sublen = (int)std::char_traits<char>::length(str) - subpos;
    Super::insert(begin() + pos, str + subpos, str + subpos + sublen);
    return *this;
}

String& String::replace(int pos, int len, const String& str, int subpos, int sublen)
{
    if (sublen == npos) sublen = str.length() - subpos;
    Super::replace(pos, len, str, subpos, sublen);
    return *this;
}

int String::compareIgnoreCase(const String& str, int pos, int len, int subpos, int sublen) const
{
    if (len == npos) len = length() - pos;
    if (sublen == npos) sublen = str.length() - subpos;

    if (pos + len > length()) len = length() - pos;
    if (subpos + sublen > str.length()) sublen = str.length() - subpos;

    int length = (len < sublen) ? len : sublen;

    for (int i = 0; i < length; ++i)
    {
        Char val = std::tolower((*this)[pos+i]);
        Char val2 = std::tolower(str[subpos+i]);
        if (val != val2) return val < val2 ? -1 : 1;
    }

    if (len != sublen) return len < sublen ? -1 : 1;
    return 0;
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
    typedef ptrdiff_t               difference_type;
    typedef String*                 pointer;
    typedef String&                 reference;
    
    Tokenizer(const String& str, const String& delim, String::const_iterator it) :
        _str(&str),
        _delim(&delim),
        _pos((int)(it - _str->begin())),
        _tokenCount(0)
    {
        assert(!_delim->empty());
        operator++();
    }

    Tokenizer& operator++()
    {
        if (_pos < 0 || _pos >= _str->length() || _pos == String::npos)
        {
            _pos = String::npos;
            _token.clear();
            return *this;
        }

        if (_tokenCount > 0)
            _pos += _delim->length();

        int lastPos = _pos;
    
        _pos = (int)_str->find(*_delim, _pos);
        if (_pos == String::npos)
            _pos = _str->length();
    
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
    
    int _pos;
    String _token;
    int _tokenCount;
};

String::List String::split(const String& delim, int pos, int count) const
{
    List ret;
    if (count == npos)
        count = length();
    std::copy(Tokenizer(*this, delim, begin()+pos), Tokenizer(*this, delim, begin()+pos+count), std::back_inserter(ret));
    return ret;
}


class Linker
{
public:
    typedef std::output_iterator_tag    iterator_category;
    typedef void                        value_type;
    typedef ptrdiff_t                   difference_type;
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
    int _token;
};

String String::join(const List& strings, const String& delim, int start, int end)
{
    auto itStart = start != npos ? strings.begin() + start : strings.begin();
    auto itEnd = end != npos ? strings.begin() + end : strings.end();

    String ret;
    std::copy(itStart, itEnd, Linker(ret, delim));
    return ret;
}

}
