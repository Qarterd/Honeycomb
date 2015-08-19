// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/String/Bytes.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{

/// \defgroup iostream  std::ios_base stream util
/// @{

/// Base class to hold iostream manipulator state.  Inherit from this class and call `Subclass::inst(ios)` to attach an instance of Subclass to an iostream.
template<class Subclass>
class Manip
{
public:
    static bool hasInst(ios_base& ios)                                  { return ios.pword(pword); }
    static Subclass& inst(ios_base& ios)
    {
        if (!hasInst(ios)) { ios.pword(pword) = new Subclass(); ios.register_callback(&Manip::delete_, 0); }
        return *static_cast<Subclass*>(ios.pword(pword));
    }
    
private:
    static void delete_(ios_base::event ev, ios_base& ios, int)         { if (ev != ios_base::erase_event) return;honey::delete_(&inst(ios)); }
    static const int pword;
};
template<class Subclass> const int Manip<Subclass>::pword = ios_base::xalloc();

/// Helper to create a manipulator that takes arguments. \see manipFunc()
template<class Func, class Tuple>
struct ManipFunc
{
    template<class Func_, class Tuple_>
    ManipFunc(Func_&& f, Tuple_&& args)                                 : f(forward<Func_>(f)), args(forward<Tuple_>(args)) {}
    
    template<class Stream>
    friend Stream& operator<<(Stream& os, const ManipFunc& manip)       { manip.apply(os, mt::make_idxseq<tuple_size<Tuple>::value>()); return os; }
    template<class Stream>
    friend Stream& operator>>(Stream& is, ManipFunc& manip)             { manip.apply(is, mt::make_idxseq<tuple_size<Tuple>::value>()); return is; }
    
    template<class Stream, size_t... Seq>
    void apply(Stream& ios, mt::idxseq<Seq...>) const                   { f(ios, get<Seq>(args)...); }
    
    Func f;
    Tuple args;
};

/// Helper to create a manipulator that takes arguments. eg. A manip named 'foo': `auto foo(int val) { return manipFunc([=](ios_base& ios) { FooManip::inst(ios).val = val; }); }`
template<class Func, class... Args>
inline auto manipFunc(Func&& f, Args&&... args)             { return ManipFunc<Func, decltype(make_tuple(forward<Args>(args)...))>(forward<Func>(f), make_tuple(forward<Args>(args)...)); }

/// @}

/// \defgroup stringstream  std::stringstream util
/// @{

/// Shorthand to create ostringstream
inline ostringstream sout()                                 { return ostringstream(); }
    
/// std::stringstream util
namespace stringstream
{
    /** \cond */
    namespace priv
    {
        struct Indent : Manip<Indent>
        {
            int level = 0;
            int size = 4;
        };
    }
    /** \endcond */

    /// Increase stream indent level by 1
    inline ostream& indentInc(ostream& os)                  { ++priv::Indent::inst(os).level; return os; }
    /// Decrease stream indent level by 1
    inline ostream& indentDec(ostream& os)                  { --priv::Indent::inst(os).level; return os; }
    /// Set number of spaces per indent level
    inline auto indentSize(int size)                        { return manipFunc([=](ostream& os) { priv::Indent::inst(os).size = size; }); }
}

/// End line and apply any indentation to the next line
inline ostream& endl(ostream& os)
{
    os << std::endl;
    if (stringstream::priv::Indent::hasInst(os))
    {
        auto& indent = stringstream::priv::Indent::inst(os);
        for (int i = 0, end = indent.level * indent.size; i < end; ++i) os << ' ';
    }
    return os;
}

/// @}

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

/// \defgroup ByteStream  ByteStream util
/// @{

/// Bool to bytes
inline ByteStream& operator<<(ByteStream& os, const bool val)   { os.put(val); return os; }
/// Byte to bytes
inline ByteStream& operator<<(ByteStream& os, const byte val)   { os.put(val); return os; }
/// Char to bytes
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

/// Bool from bytes
inline ByteStream& operator>>(ByteStream& is, bool& val)        { val = is.get(); return is; }
/// Byte from bytes
inline ByteStream& operator>>(ByteStream& is, byte& val)        { is.get(reinterpret_cast<char&>(val)); return is; }
/// Char from bytes
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

/** \cond */
namespace priv
{
    template<class Tuple, size_t... Seq>
    void tupleToBytes(ByteStream& os, Tuple&& t, mt::idxseq<Seq...>)
                                                                { mt::exec([&]() { os << get<Seq>(forward<Tuple>(t)); }...); }
    template<class Tuple, size_t... Seq>
    void tupleFromBytes(ByteStream& is, Tuple& t, mt::idxseq<Seq...>)
                                                                { mt::exec([&]() { is >> get<Seq>(t); }...); }
}
/** \endcond */

/// Tuple to bytes
template<class Tuple>
typename std::enable_if<mt::isTuple<Tuple>::value, ByteStream&>::type
    operator<<(ByteStream& os, Tuple&& t)                       { priv::tupleToBytes(os, forward<Tuple>(t), mt::make_idxseq<tuple_size<typename mt::removeRef<Tuple>::type>::value>()); return os; }
/// Tuple from bytes
template<class Tuple>
typename std::enable_if<mt::isTuple<Tuple>::value, ByteStream&>::type
    operator>>(ByteStream& is, Tuple& t)                        { priv::tupleFromBytes(is, t, mt::make_idxseq<tuple_size<Tuple>::value>()); return is; }
    
/// ByteStream util
namespace bytestream
{
    
}
/// @}

}

/** \cond */
namespace std
{
/** \endcond */
    /// \ingroup stringstream
    /// @{
    
    /// Pair to string
    template<class T1, class T2>
    ostream& operator<<(ostream& os, const pair<T1,T2>& p)  { return os << "[" << p.first << ", " << p.second << "]"; }
    
    /** \cond */
    namespace priv
    {
        template<class Tuple, size_t... Seq>
        void tupleToString(ostream& os, Tuple&& t, honey::mt::idxseq<Seq...>)
                                                            { os << "["; honey::mt::exec([&]() { os << get<Seq>(forward<Tuple>(t)) << (Seq < sizeof...(Seq)-1 ? ", " : ""); }...); os << "]"; }
        template<class List>
        void listToString(ostream& os, const List& list)    { int i = 0; os << "["; for (auto& e: list) os << (i++ > 0 ? ", " : "") << e; os << "]"; }
    }
    /** \endcond */

    /// Tuple to string
    template<class Tuple>
    typename enable_if<honey::mt::isTuple<Tuple>::value, ostream&>::type
        operator<<(ostream& os, Tuple&& t)                  { priv::tupleToString(os, forward<Tuple>(t), honey::mt::make_idxseq<tuple_size<typename honey::mt::removeRef<Tuple>::type>::value>()); return os; }
    
    /// Array to string
    template<class T, size_t N>
    ostream& operator<<(ostream& os, const array<T,N>& a)   { priv::listToString(os, a); return os; }
    /// Vector to string
    template<class T, class Alloc>
    ostream& operator<<(ostream& os, const vector<T,Alloc>& vec)
                                                            { priv::listToString(os, vec); return os; }
    /// Set to string
    template<class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const set<T,Compare,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Multi-Set to string
    template<class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const multiset<T,Compare,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Unordered Set to string
    template<class Key, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_set<Key,Hash,KeyEqual,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Unordered Multi-Set to string
    template<class Key, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_multiset<Key,Hash,KeyEqual,Alloc>& set)
                                                            { priv::listToString(os, set); return os; }
    /// Map to string
    template<class Key, class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const map<Key,T,Compare,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Multi-Map to string
    template<class Key, class T, class Compare, class Alloc>
    ostream& operator<<(ostream& os, const multimap<Key,T,Compare,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Unordered Map to string
    template<class Key, class T, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_map<Key,T,Hash,KeyEqual,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// Unordered Multi-Map to string
    template<class Key, class T, class Hash, class KeyEqual, class Alloc>
    ostream& operator<<(ostream& os, const unordered_multimap<Key,T,Hash,KeyEqual,Alloc>& map)
                                                            { priv::listToString(os, map); return os; }
    /// @}
/** \cond */
}
/** \endcond */

