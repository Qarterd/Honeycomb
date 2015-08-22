// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/String/Bytes.h"
#include "Honey/Misc/StdUtil.h"
#include "Honey/Memory/SharedPtr.h"

namespace honey
{

/// \defgroup ByteStream  ByteStream util
/// @{

/// A stream I/O buffer of bytes, to be passed into ByteStream
class ByteBuf : public std::stringbuf
{
public:
    typedef std::stringbuf Super;
    
    explicit ByteBuf(ios_base::openmode mode = 0)
                                                            : Super(ios_base::in|ios_base::out|mode), _mode(mode) {}
    explicit ByteBuf(const Bytes& bs, ios_base::openmode mode = 0)
                                                            : ByteBuf(mode) { bytes(bs); }
    ByteBuf(ByteBuf&& rhs)                                  : Super(move(rhs)), _mode(rhs._mode) {}
    
    ByteBuf& operator=(ByteBuf&& rhs)                       { Super::operator=(move(rhs)); _mode = rhs._mode; return *this; }
    
    Bytes bytes() const                                     { return Bytes(reinterpret_cast<const byte*>(pbase()), reinterpret_cast<const byte*>(egptr() > pptr() ? egptr() : pptr())); }
    void bytes(const Bytes& bs)
    {
        seekoff(0, ios_base::beg, ios_base::out);
        sputn(reinterpret_cast<const char*>(bs.data()), bs.size());
        setg(pbase(), pbase(), pptr());
        if (!appendMode()) seekoff(0, ios_base::beg, ios_base::out);
    }
    
private:
    std::basic_string<byte> str() const;
    void str(const std::basic_string<byte>& s);
    bool appendMode() const                                 { return _mode & (ios_base::app | ios_base::ate); }
    
    ios_base::openmode _mode;
};

/// An I/O stream into which objects may be serialized and subsequently deserialized
class ByteStream : public std::iostream
{
public:
    typedef std::iostream Super;
    
    explicit ByteStream(std::streambuf* sb)                 : Super(sb) {}
    ByteStream(ByteStream&& rhs)                            : Super(std::move(rhs)) {}
    
    ByteStream& operator=(ByteStream&& rhs)                 { Super::operator=(std::move(rhs)); return *this; }
};

/// Bool to bytes
inline ByteStream& operator<<(ByteStream& os, const bool val)   { os.put(val); return os; }
/// Byte to bytes
inline ByteStream& operator<<(ByteStream& os, const byte val)   { os.put(val); return os; }
/// UTF-8 char to bytes
inline ByteStream& operator<<(ByteStream& os, const char val)   { os.put(val); return os; }
/// Multi-byte number to big-endian bytes
template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type=0>
ByteStream& operator<<(ByteStream& os, const T val)
{
    byte a[sizeof(T)];
    BitOp::toPartsBig(val, a);
    os.write(reinterpret_cast<char*>(a), sizeof(T));
    return os;
}
/// Char to bytes
inline ByteStream& operator<<(ByteStream& os, Char val)         { return os << uint16(val); }
    
/// Bool from bytes
inline ByteStream& operator>>(ByteStream& is, bool& val)        { val = is.get(); return is; }
/// Byte from bytes
inline ByteStream& operator>>(ByteStream& is, byte& val)        { is.get(reinterpret_cast<char&>(val)); return is; }
/// UTF-8 char from bytes
inline ByteStream& operator>>(ByteStream& is, char& val)        { is.get(val); return is; }
/// Multi-byte number from big-endian bytes
template<class T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, int>::type=0>
ByteStream& operator>>(ByteStream& is, T& val)
{
    byte a[sizeof(T)];
    is.read(reinterpret_cast<char*>(a), sizeof(T));
    val = BitOp::fromPartsBig<T>(a);
    return is;
}
/// Char from bytes
inline ByteStream& operator>>(ByteStream& is, Char& val)        { uint16 c; is >> c; val = Char(c); return is; }

/// ByteStream util
namespace bytestream
{
    /** \cond */
    namespace priv
    {
        template<class Int>
        struct VarSize
        {
            friend ByteStream& operator<<(ByteStream& os, const VarSize& v)
            {
                assert(v.val >= 0, "VarSize value must be positive");
                if (v.val <= 0)                             return os << uint8(0);
                else if (v.val <= numeral<uint8>().max()-3) return os << uint8(v.val);
                else if (v.val <= numeral<uint16>().max())  return os << numeral<uint8>().max() << uint16(v.val);
                else if (v.val <= numeral<uint32>().max())  return os << uint8(numeral<uint8>().max()-1) << uint32(v.val);
                else                                        return os << uint8(numeral<uint8>().max()-2) << uint64(v.val);
            }
            
            template<class VarSize_>
            friend typename std::enable_if<std::is_same<VarSize,VarSize_>::value, ByteStream&>::type
                operator>>(ByteStream& is, const VarSize_& v)
            {
                static_assert(mt::isLref<Int>::value && !std::is_const<typename mt::removeRef<Int>::type>::value, "VarSize requires a mutable integer reference for extraction");
                typedef typename std::remove_const<typename mt::removeRef<Int>::type>::type Int_;
                
                uint8 size; is >> size;
                switch (size)
                {
                case numeral<uint8>().max():    { uint16 val; is >> val; v.val = numeric_cast<Int_>(val); break; }
                case numeral<uint8>().max()-1:  { uint32 val; is >> val; v.val = numeric_cast<Int_>(val); break; }
                case numeral<uint8>().max()-2:  { uint64 val; is >> val; v.val = numeric_cast<Int_>(val); break; }
                default:                        v.val = numeric_cast<Int_>(size); break;
                }
                return is;
            }
            
            Int val;
        };
    }
    /** \endcond */
    
    /// Write or read a size (a positive integer) using a minimal number of bytes
    template<class Int, typename std::enable_if<std::is_integral<typename mt::removeRef<Int>::type>::value, int>::type=0>
    inline auto varSize(Int&& val)                              { return priv::VarSize<Int>{forward<Int>(val)}; }
}

/// Pair to bytes
template<class T1, class T2>
ByteStream& operator<<(ByteStream& os, const pair<T1,T2>& p)    { return os << p.first << p.second; }
/** \cond */
namespace priv
{
    template<class Tuple, szt... Seq>
    void tupleToBytes(ByteStream& os, Tuple&& t, mt::idxseq<Seq...>)
                                                                { mt::exec([&]() { os << get<Seq>(forward<Tuple>(t)); }...); }
    template<class List>
    void listToBytes(ByteStream& os, const List& list)          { os << bytestream::varSize(list.size()); for (auto& e: list) os << e; }
}
/** \endcond */
/// Tuple to bytes
template<class Tuple>
typename std::enable_if<mt::isTuple<Tuple>::value, ByteStream&>::type
    operator<<(ByteStream& os, Tuple&& t)               { priv::tupleToBytes(os, forward<Tuple>(t), mt::make_idxseq<tuple_size<typename mt::removeRef<Tuple>::type>::value>()); return os; }
/// Array to bytes
template<class T, szt N>
ByteStream& operator<<(ByteStream& os, const array<T,N>& a)
                                                        { for (auto& e: a) os << e; return os; }
/// Vector to bytes
template<class T, class Alloc>
ByteStream& operator<<(ByteStream& os, const vector<T,Alloc>& vec)
                                                        { priv::listToBytes(os, vec); return os; }
/// UTF-8 string to bytes
inline ByteStream& operator<<(ByteStream& os, const char* str)
                                                        { auto len = strlen(str); os << bytestream::varSize(len); os.write(str, len); return os; }
/// UTF-8 string to bytes
inline ByteStream& operator<<(ByteStream& os, const std::string& str)
                                                        { os << bytestream::varSize(str.length()); os.write(str.data(), str.length()); return os; }
/// String to bytes
inline ByteStream& operator<<(ByteStream& os, const String& str)
                                                        { priv::listToBytes(os, str); return os; }
/// Bytes to bytes
inline ByteStream& operator<<(ByteStream& os, const Bytes& bs)
                                                        { os << bytestream::varSize(bs.size()); os.write(reinterpret_cast<const char*>(bs.data()), bs.size()); return os; }
/// Set to bytes
template<class T, class Compare, class Alloc>
ByteStream& operator<<(ByteStream& os, const set<T,Compare,Alloc>& set)
                                                        { priv::listToBytes(os, set); return os; }
/// Multi-Set to bytes
template<class T, class Compare, class Alloc>
ByteStream& operator<<(ByteStream& os, const multiset<T,Compare,Alloc>& set)
                                                        { priv::listToBytes(os, set); return os; }
/// Unordered Set to bytes
template<class Key, class Hash, class KeyEqual, class Alloc>
ByteStream& operator<<(ByteStream& os, const unordered_set<Key,Hash,KeyEqual,Alloc>& set)
                                                        { priv::listToBytes(os, set); return os; }
/// Unordered Multi-Set to bytes
template<class Key, class Hash, class KeyEqual, class Alloc>
ByteStream& operator<<(ByteStream& os, const unordered_multiset<Key,Hash,KeyEqual,Alloc>& set)
                                                        { priv::listToBytes(os, set); return os; }
/// Map to bytes
template<class Key, class T, class Compare, class Alloc>
ByteStream& operator<<(ByteStream& os, const std::map<Key,T,Compare,Alloc>& map)
                                                        { priv::listToBytes(os, map); return os; }
/// Multi-Map to bytes
template<class Key, class T, class Compare, class Alloc>
ByteStream& operator<<(ByteStream& os, const multimap<Key,T,Compare,Alloc>& map)
                                                        { priv::listToBytes(os, map); return os; }
/// Unordered Map to bytes
template<class Key, class T, class Hash, class KeyEqual, class Alloc>
ByteStream& operator<<(ByteStream& os, const unordered_map<Key,T,Hash,KeyEqual,Alloc>& map)
                                                        { priv::listToBytes(os, map); return os; }
/// Unordered Multi-Map to bytes
template<class Key, class T, class Hash, class KeyEqual, class Alloc>
ByteStream& operator<<(ByteStream& os, const unordered_multimap<Key,T,Hash,KeyEqual,Alloc>& map)
                                                        { priv::listToBytes(os, map); return os; }

/// Pair from bytes
template<class T1, class T2>
ByteStream& operator>>(ByteStream& is, pair<T1,T2>& p)  { return is >> p.first >> p.second; }
/** \cond */
namespace priv
{
    template<class Tuple, szt... Seq>
    void tupleFromBytes(ByteStream& is, Tuple& t, mt::idxseq<Seq...>)
                                                        { mt::exec([&]() { is >> get<Seq>(t); }...); }
    template<class Set, class T = typename Set::value_type>
    void setFromBytes(ByteStream& is, Set& set)         { szt size; is >> bytestream::varSize(size); for (auto _: range(size)) { mt_unused(_); T val; is >> val; set.insert(val); } }
    template<class Map, class Key = typename Map::key_type, class Val = typename Map::mapped_type>
    void mapFromBytes(ByteStream& is, Map& map)         { szt size; is >> bytestream::varSize(size); for (auto _: range(size)) { mt_unused(_); Key key; is >> key; Val val; is >> val; map.insert(make_pair(key, val)); } }
}
/** \endcond */
/// Tuple from bytes
template<class Tuple>
typename std::enable_if<mt::isTuple<Tuple>::value, ByteStream&>::type
    operator>>(ByteStream& is, Tuple& t)                { priv::tupleFromBytes(is, t, mt::make_idxseq<tuple_size<Tuple>::value>()); return is; }
/// Array from bytes
template<class T, szt N>
ByteStream& operator>>(ByteStream& is, array<T,N>& a)
                                                        { for (auto& e: a) is >> e; return is; }
/// Vector from bytes
template<class T, class Alloc>
ByteStream& operator>>(ByteStream& is, vector<T,Alloc>& vec)
                                                        { szt size; is >> bytestream::varSize(size); vec.resize(size); for (auto& e: vec) is >> e; return is; }
/// UTF-8 string from bytes
inline ByteStream& operator>>(ByteStream& is, std::string& str)
                                                        { szt size; is >> bytestream::varSize(size); str.resize(size); is.read(&str[0], str.length()); return is; }
/// String from bytes
inline ByteStream& operator>>(ByteStream& is, String& str)
                                                        { szt size; is >> bytestream::varSize(size); str.resize(size); for (auto& e: str) is >> e; return is; }
/// Bytes from bytes
inline ByteStream& operator>>(ByteStream& is, Bytes& bs)
                                                        { szt size; is >> bytestream::varSize(size); bs.resize(size); is.read(reinterpret_cast<char*>(bs.data()), bs.size()); return is; }
/// Set from bytes
template<class T, class Compare, class Alloc>
ByteStream& operator>>(ByteStream& is, set<T,Compare,Alloc>& set)
                                                        { priv::setFromBytes(is, set); return is; }
/// Multi-Set from bytes
template<class T, class Compare, class Alloc>
ByteStream& operator>>(ByteStream& is, multiset<T,Compare,Alloc>& set)
                                                        { priv::setFromBytes(is, set); return is; }
/// Unordered Set from bytes
template<class Key, class Hash, class KeyEqual, class Alloc>
ByteStream& operator>>(ByteStream& is, unordered_set<Key,Hash,KeyEqual,Alloc>& set)
                                                        { priv::setFromBytes(is, set); return is; }
/// Unordered Multi-Set from bytes
template<class Key, class Hash, class KeyEqual, class Alloc>
ByteStream& operator>>(ByteStream& is, unordered_multiset<Key,Hash,KeyEqual,Alloc>& set)
                                                        { priv::setFromBytes(is, set); return is; }
/// Map from bytes
template<class Key, class T, class Compare, class Alloc>
ByteStream& operator>>(ByteStream& is, std::map<Key,T,Compare,Alloc>& map)
                                                        { priv::mapFromBytes(is, map); return is; }
/// Multi-Map from bytes
template<class Key, class T, class Compare, class Alloc>
ByteStream& operator>>(ByteStream& is, multimap<Key,T,Compare,Alloc>& map)
                                                        { priv::mapFromBytes(is, map); return is; }
/// Unordered Map from bytes
template<class Key, class T, class Hash, class KeyEqual, class Alloc>
ByteStream& operator>>(ByteStream& is, unordered_map<Key,T,Hash,KeyEqual,Alloc>& map)
                                                        { priv::mapFromBytes(is, map); return is; }
/// Unordered Multi-Map from bytes
template<class Key, class T, class Hash, class KeyEqual, class Alloc>
ByteStream& operator>>(ByteStream& is, unordered_multimap<Key,T,Hash,KeyEqual,Alloc>& map)
                                                        { priv::mapFromBytes(is, map); return is; }

/// @}

}

