// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/String/Hash.h"

namespace honey
{

/**
  * \defgroup Id    String Identifier
  *
  * String ids provide a fast way to compare strings. \n
  * An Id is composed of a name string and its hashed integral value (using hash::fast()). \n
  * In final mode an Id only holds the hash; name() and operator<<() are not available.
  *
  * \see string literal operator `_id` to create ids at compile-time.
  */
/// @{

class IdLiteral;
class ByteStream;

/// Holds a name string and its hashed value for fast comparison ops. See \ref Id
class Id
{
    friend class IdLiteral;
public:
    Id()                                            : _hash(0) {}
    Id(const String& name)                          : debug_if(_name(name),) _hash(hash::fast(name)) {}
    /// Construct with precalculated hash
    Id(const String& name, int hash)                : debug_if(_name(name),) _hash(hash) { assert(_hash == hash::fast(_name)); }
    Id(const IdLiteral& rhs);
    Id(const Id& rhs)                               : debug_if(_name(rhs._name),) _hash(rhs._hash) {}
    Id(Id&& rhs)                                    : debug_if(_name(move(rhs._name)),) _hash(rhs._hash) {}
    
    Id& operator=(const Id& rhs)                    { debug_if(_name = rhs._name;) _hash = rhs._hash; return *this; }
    Id& operator=(Id&& rhs)                         { debug_if(_name = move(rhs._name);) _hash = rhs._hash; return *this; }
    Id& operator=(const IdLiteral& rhs);
    
    bool operator==(const Id& rhs) const            { return _hash == rhs._hash; }
    bool operator!=(const Id& rhs) const            { return _hash != rhs._hash; }
    bool operator< (const Id& rhs) const            { return _hash < rhs._hash; }
    bool operator> (const Id& rhs) const            { return _hash > rhs._hash; }
    bool operator<=(const Id& rhs) const            { return _hash <= rhs._hash; }
    bool operator>=(const Id& rhs) const            { return _hash >= rhs._hash; }

    bool operator==(const IdLiteral& rhs) const;
    bool operator!=(const IdLiteral& rhs) const;
    bool operator< (const IdLiteral& rhs) const;
    bool operator> (const IdLiteral& rhs) const;
    bool operator<=(const IdLiteral& rhs) const;
    bool operator>=(const IdLiteral& rhs) const;
    
    #ifndef FINAL
        /// Get name string that this id represents
        const String& name() const                  { return _name; }
    #endif
    /// Get hashed integral value of name
    int hash() const                                { return _hash; }
    
    /// Same as hash()
    operator int() const                            { return _hash; }

    #ifndef FINAL
        friend ostream& operator<<(ostream& os, const Id& val)      { return val._hash ? (val._name.length() ? os << val._name : os << val._hash) : os << "idnull"; }
    #else
        friend ostream& operator<<(ostream& os, const Id& val)      { return val._hash ? os << val._hash : os << "idnull"; }
    #endif
    friend ByteStream& operator<<(ByteStream& os, const Id& val);
    friend ByteStream& operator>>(ByteStream& is, Id& val);
    
protected:
    #ifndef FINAL
        String _name;
    #endif
    int _hash;
};

/// Id created from a string literal at compile-time. \see string literal operator `_id`
class IdLiteral
{
    friend class Id;
public:
    constexpr IdLiteral()                           : debug_if(_name(""),) _hash(0) {}
    constexpr IdLiteral(const char* str, szt len)   : debug_if(_name(str),) _hash(hash::fast_(str, len)) {}
    
    constexpr bool operator==(const IdLiteral& rhs) const   { return _hash == rhs._hash; }
    constexpr bool operator!=(const IdLiteral& rhs) const   { return _hash != rhs._hash; }
    constexpr bool operator< (const IdLiteral& rhs) const   { return _hash < rhs._hash; }
    constexpr bool operator> (const IdLiteral& rhs) const   { return _hash > rhs._hash; }
    constexpr bool operator<=(const IdLiteral& rhs) const   { return _hash <= rhs._hash; }
    constexpr bool operator>=(const IdLiteral& rhs) const   { return _hash >= rhs._hash; }
    
    bool operator==(const Id& rhs) const            { return _hash == rhs._hash; }
    bool operator!=(const Id& rhs) const            { return _hash != rhs._hash; }
    bool operator< (const Id& rhs) const            { return _hash < rhs._hash; }
    bool operator> (const Id& rhs) const            { return _hash > rhs._hash; }
    bool operator<=(const Id& rhs) const            { return _hash <= rhs._hash; }
    bool operator>=(const Id& rhs) const            { return _hash >= rhs._hash; }
    
    #ifndef FINAL
        constexpr const char* name() const          { return _name; }
    #endif
    constexpr int hash() const                      { return _hash; }
    
    constexpr operator int() const                  { return _hash; }
    
    #ifndef FINAL
        friend ostream& operator<<(ostream& os, const IdLiteral& val)   { return val._hash ? os << val._name : os << "idnull"; }
    #else
        friend ostream& operator<<(ostream& os, const IdLiteral& val)   { return val._hash ? os << val._hash : os << "idnull"; }
    #endif
    friend ByteStream& operator<<(ByteStream& os, const IdLiteral& val);
    
private:
    #ifndef FINAL
        const char* _name;
    #endif
    int _hash;
};

/// Null id
#define idnull IdLiteral()

/// Create an id from a string literal at compile-time.
/**
  * This operator can be used in a case expression of a switch block (ex. case "foo"_id: ).
  */ 
constexpr IdLiteral operator"" _id(const char* str, szt len)     { return IdLiteral(str, len); }

/** \cond */
inline Id::Id(const IdLiteral& rhs)                         : debug_if(_name(rhs._name),) _hash(rhs._hash) {}
inline Id& Id::operator=(const IdLiteral& rhs)              { debug_if(_name = rhs._name;) _hash = rhs._hash; return *this; }
inline bool Id::operator==(const IdLiteral& rhs) const      { return _hash == rhs._hash; }
inline bool Id::operator!=(const IdLiteral& rhs) const      { return _hash != rhs._hash; }
inline bool Id::operator<=(const IdLiteral& rhs) const      { return _hash <= rhs._hash; }
inline bool Id::operator>=(const IdLiteral& rhs) const      { return _hash >= rhs._hash; }
inline bool Id::operator< (const IdLiteral& rhs) const      { return _hash < rhs._hash; }
inline bool Id::operator> (const IdLiteral& rhs) const      { return _hash > rhs._hash; }
/** \endcond */

/// Holds both a name string and its hashed value, and unlike Id the name is never compiled out.
class NameId : public Id
{
public:
    NameId() = default;
    NameId(const String& name)                      : Id(name), _name(name) {}
    NameId(const char* name)                        : Id(name), _name(name) {}
    /// Create with id, leaving name empty
    explicit NameId(const Id& id)                   : Id(id) {}
    /// Create with name and precalculated id
    NameId(const String& name, const Id& id)        : Id(id), _name(name) { assert(_name == Id::name()); }
    
    const String& name() const                      { return _name; }
    
    friend ostream& operator<<(ostream& os, const NameId& val)          { return os << val._name; }
    friend ByteStream& operator<<(ByteStream& os, const NameId& val);
    friend ByteStream& operator>>(ByteStream& is, NameId& val);
    
private:
    String _name;
};
/// @}

}

/** \cond */
/// Allow class to be used as key in unordered containers
template<>
struct std::hash<honey::Id>
{
    size_t operator()(const honey::Id& val) const           { return val.hash(); }
};

template<>
struct std::hash<honey::IdLiteral>
{
    size_t operator()(const honey::IdLiteral& val) const    { return val.hash(); }
};

template<>
struct std::hash<honey::NameId>
{
    size_t operator()(const honey::NameId& val) const       { return val.hash(); }
};
/** \endcond */

