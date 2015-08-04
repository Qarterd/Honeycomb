// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Id.h"
#include "Honey/Misc/Variant.h"
#include "Honey/Memory/SmallAllocator.h"

namespace honey
{
/// Json string serialization format methods
namespace json
{

/// Json value type, corresponds to Value::Variant bounded type id
enum class ValueType                                { Null, Int, Real, Bool, String, Array, Object };
/// Json null value type
struct null_t {};
/// Json null value
static null_t null;

struct ValueError : Exception                       { EXCEPTION(ValueError) };
    
template<class Config> class Value_;

/// Configuration for json value variant
/**
  * \tparam ordered     If true then insertion order of name/value pairs in a json object will be preserved
  *                     in an additional list member `Value::Object::orderedNames`. \see ObjectOrdered.
  */
template<bool ordered = false>
struct Config
{
    typedef Value_<Config> Value;
    /// Holds a value in an array/object
    typedef recursive_wrap<Value, SmallAllocator<Value>> value_wrap;
    typedef vector<value_wrap, SmallAllocator<value_wrap>> Array;
    typedef stdutil::unordered_map<NameId, value_wrap, SmallAllocator> ObjectUnordered;
    
    struct ObjectOrdered : ObjectUnordered
    {
        typedef typename ObjectUnordered::iterator Iter;
        typedef typename ObjectUnordered::const_iterator ConstIter;
        
        template<class Pair>
        pair<Iter, bool> insert(Pair&& pair)
        {
            auto res = ObjectUnordered::insert(forward<Pair>(pair));
            if (res.second) orderedNames.push_back(res.first->first);
            return res;
        }
        
        /// This function is O(n)
        Iter erase(ConstIter pos)                   { stdutil::eraseVal(orderedNames, pos->first); return ObjectUnordered::erase(pos); }
        void clear()                                { orderedNames.clear(); ObjectUnordered::clear(); }
        
        /// List of names in order of name/value pair insertion
        vector<Id, SmallAllocator<Id>> orderedNames;
    };
    
    typedef typename std::conditional<ordered, ObjectOrdered, ObjectUnordered>::type Object;
    
    typedef variant<null_t, int64, double, bool, String, Array, Object> Variant;
};

/// Json value variant.  Provide a Config to customize the variant's bounded types.
template<class Config_>
class Value_ : public Config_::Variant
{
public:
    typedef Config_ Config;
    /// Json value array
    typedef typename Config::Array Array;
    /// Json object, map of json name/value pairs.  May be ObjectUnordered (default) or ObjectOrdered depending on config.
    typedef typename Config::Object Object;
    /// Object with unordered name/value pairs
    typedef typename Config::ObjectUnordered ObjectUnordered;
    /// Object with ordered name/value pairs.  \see ObjectOrdered::orderedNames.
    typedef typename Config::ObjectOrdered ObjectOrdered;
    /// Base class, the underlying variant
    typedef typename Config::Variant Variant;
    
    /// Construct with null value
    Value_()                                        : Value_(null) {}
    /// Attempts to copy/move construct any json value type, otherwise first json value type constructible with `val` is set as the value.
    template<class T>
    Value_(T&& val)                                 : Variant(forward<T>(val)) {}
    Value_(const Value_& rhs)                       : Value_(static_cast<const Variant&>(rhs)) {}
    Value_(Value_& rhs)                             : Value_(static_cast<Variant&>(rhs)) {}
    Value_(Value_&& rhs)                            : Value_(move(static_cast<Variant&>(rhs))) {}
    
    /// Attempts to copy/move-assign any json value type, otherwise first json value type assignable to `val` is set as the value.
    template<class T>
    Value_& operator=(T&& val)                      { Variant::operator=(forward<T>(val)); return *this; }
    Value_& operator=(const Value_& rhs)            { Variant::operator=(static_cast<const Variant&>(rhs)); return *this; }
    Value_& operator=(Value_& rhs)                  { Variant::operator=(static_cast<Variant&>(rhs)); return *this; }
    Value_& operator=(Value_&& rhs)                 { Variant::operator=(move(static_cast<Variant&>(rhs))); return *this; }
    
    /// Get value at index in array
    Value_& operator[](int i)
    {
        assert(type() == ValueType::Array);
        auto& arr = get_<Array>();
        assert(i >= 0 && i < honey::size(arr));
        return arr[i];
    }
    const Value_& operator[](int i) const           { return const_cast<Value_&>(*this)[i]; }
    
    /// Get value with name in object.  Adds null value if it doesn't exist.
    Value_& operator[](const String& name)
    {
        assert(type() == ValueType::Object);
        auto& obj = get_<Object>();
        Id id = name;
        auto it = obj.find(NameId(id));
        if (it == obj.end()) it = obj.insert(make_pair(NameId(name, id), Value_())).first;
        return it->second;
    }
    //resolves ambiguity between String and built-in asymmetric operator[](int, const Char*)
    Value_& operator[](const Char* name)            { return (*this)[String(name)]; }
    Value_& operator[](const char* name)            { return (*this)[String(name)]; }
    
    /// Get value with id in object.  Throws ValueError if it doesn't exist.
    Value_& operator[](const Id& id)
    {
        assert(type() == ValueType::Object);
        auto& obj = get_<Object>();
        auto it = obj.find(NameId(id));
        if (it == obj.end()) throw_ ValueError() << "Value not found. Id: " debug_if(<< id);
        return it->second;
    }
    const Value_& operator[](const Id& id) const    { return const_cast<Value_&>(this)[id]; }
    //resolves ambiguity between Id and int
    Value_& operator[](const IdLiteral& id)             { return (*this)[Id(id)]; }
    const Value_& operator[](const IdLiteral& id) const { return (*this)[Id(id)]; }
    
    /// Get active json value type
    ValueType type() const                          { return static_cast<ValueType>(Variant::type()); }
    
    /// Get iterator to first value in array
    typename Array::iterator begin()                { assert(type() == ValueType::Array); return get_<Array>().begin(); }
    /// Get iterator to first value in array
    typename Array::const_iterator begin() const    { assert(type() == ValueType::Array); return get_<Array>().begin(); }
    
    /// Get iterator to position after the last value in array
    typename Array::iterator end()                  { assert(type() == ValueType::Array); return get_<Array>().end(); }
    /// Get iterator to position after the last value in array
    typename Array::const_iterator end() const      { assert(type() == ValueType::Array); return get_<Array>().end(); }
    
    /// Get first value in array
    Value_& front()                                 { assert(!empty()); return get_<Array>().front(); }
    const Value_& front() const                     { assert(!empty()); return get_<Array>().front(); }
    /// Get last value in array
    Value_& back()                                  { assert(!empty()); return get_<Array>().back(); }
    const Value_& back() const                      { assert(!empty()); return get_<Array>().back(); }
    
    /// Create Value from `val` and add value to back of array
    template<class T>
    void push_back(T&& val)                         { assert(type() == ValueType::Array); get_<Array>().push_back(Value_(forward<T>(val))); }
    /// Remove last value from array
    void pop_back()                                 { assert(type() == ValueType::Array); get_<Array>().pop_back(); }
    
    /// Check if object contains value with id
    bool contains(const Id& id)                     { assert(type() == ValueType::Object); return get_<Object>().count(NameId(id)); }
    
    /// Create Value from `val` and insert value into array at index
    template<class T>
    void insert(int i, T&& val)
    {
        assert(type() == ValueType::Array);
        auto& arr = get_<Array>();
        assert(i >= 0 && i <= honey::size(arr));
        arr.insert(arr.begin() + i, Value_(forward<T>(val)));
    }
    
    /// Create Value from `val` and insert name/value pair into object.
    /**
      * \retval pair-iter   iterator to new name/value pair in object on success, otherwise iterator to existing pair in object
      * \retval success     true if name is unique and value was inserted
      */
    template<class T>
    pair<typename Object::iterator, bool> insert(const String& name, T&& val)
    {
        assert(type() == ValueType::Object);
        return get_<Object>().insert(make_pair(NameId(name), Value_(forward<T>(val))));
    }
    
    /// Erase value at index in array
    void erase(int i)
    {
        assert(type() == ValueType::Array);
        auto& arr = get_<Array>();
        assert(i >= 0 && i < honey::size(arr));
        arr.erase(arr.begin() + i);
    }
    
    /// Erase value with id in object.  Returns true if found and erased.
    bool erase(const Id& id)
    {
        assert(type() == ValueType::Object);
        auto& obj = get_<Object>();
        auto it = obj.find(NameId(id));
        if (it == obj.end()) return false;
        obj.erase(it);
        return true;
    }
    bool erase(const IdLiteral& id)                 { return erase(Id(id)); }
    
    /// Clear all values in array/object
    void clear()
    {
        assert(type() == ValueType::Array || type() == ValueType::Object);
        return visit(overload(
            [](Array& arr) { arr.clear(); },
            [](Object& obj) { obj.clear(); }
        ));
    }
    
    /// Check if array/object contains any values
    bool empty() const                              { return !size(); }
    
    /// Get number of values in array/object
    int size() const
    {
        assert(type() == ValueType::Array || type() == ValueType::Object);
        return this->template visit<int>(overload(
            [](const Array& arr) { return honey::size(arr); },
            [](const Object& obj) { return honey::size(obj); }
        ));
    }
    
private:    
    /// Shorthand
    template<class T> T& get_()                     { return this->template get<T>(); }
    template<class T> const T& get_() const         { return this->template get<T>(); }
};

/// Json value with default config
typedef Value_<Config<>> Value;

/// Convert a string to a json value tree.  Returns either a tree with `val` as the root array/object, or null if string is empty.
/**
  * All whitespace before the root json value (if any) is consumed.
  * After parsing the stream position will be just after the root json array end ']' or object end '}'.
  * Throws ValueError on parse failure.
  */
template<class Config> istream& operator>>(istream& is, Value_<Config>& val);

/** \cond */
namespace priv
{
    struct WriterManip : Manip<WriterManip>
    {
        bool beautify = false;
        bool escapeSlash = false;
    };
}
/** \endcond */

/// Output easily readable json with line breaks and indentation
inline ostream& beautify(ostream& os)               { priv::WriterManip::inst(os).beautify = true; return os; }
/// Escape forward slash characters in json output so it is suitable for embedding in HTML
inline ostream& escapeSlash(ostream& os)            { priv::WriterManip::inst(os).escapeSlash = true; return os; }

/// Convert a json value tree to a string.  Throws ValueError on write failure.
template<class Config> ostream& operator<<(ostream& os, const Value_<Config>& val);

}

extern template class Manip<json::priv::WriterManip>;

}