// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Id.h"
#include "Honey/Misc/Optional.h"

namespace honey
{

/// A compile-time associative heterogeneous container.
/**
    \defgroup MtMap     Meta-map

    Each value may have a different type.
    Lookups are resolved at compile-time, so they are O(1) constant-time.
            
    Features:
        - Supports all types including constants and references
        - Keys can be required or optional on construction (fails at compile-time)
        - Keys can be initialized in any order, keys not in the map are ignored
        - The map can be copy/move assigned to any other map, non matching keys are ignored

    For unwieldly functions with many arguments a map is ideal to enable "keyword arguments", see the example. \n
    The map can also be used as an indexable array, see mtkeygen and the example.

    __Example:__ \include MtMap.cpp
  */
/// @{

/// \example MtMap.cpp  Meta-map usage

template<class Key_, class Val_, class List_> class MtMapElem;
/// Tail of map list
typedef MtMapElem<mt::Void, mt::Void, mt::Void> MtMapTail;

/** \cond */
namespace priv
{
    template<class... Pairs> struct MtMap_                          { typedef MtMapTail type; };
    template<class Val, class Key, class... Pairs>
    struct MtMap_<Val, Key, Pairs...>                               { typedef MtMapElem<Key, Val, typename MtMap_<Pairs...>::type> type; };
}
/** \endcond */

/// Declare a map type with `MtMap<Val1,Key1, Val2,Key2, ...>`
template<class... Pairs> using MtMap                                = typename priv::MtMap_<Pairs...>::type;

/// Key/value pair.  A pair can be constructed with the syntax: `(key() = value)`
template<class Key_, class Val_>
struct MtPair
{
    typedef Key_ Key;
    typedef Val_ Val;
    
    template<class Val>
    MtPair(Val&& val)                                               : key(), val(forward<Val>(val)) {}

    const Key key;
    Val val;
};

/// Construct a key type with name
#define mtkey(Name)                                                             \
    struct Name                                                                 \
    {                                                                           \
        /** Id can be accessed with id() */                                     \
        static mt_global(const Id, id, (#Name));                                \
        /** Generate pairs with any value. Pair may store ref to rvalue. */     \
        template<class Val> MtPair<Name, Val> operator=(Val&& val)              \
        { return MtPair<Name, Val>(forward<Val>(val)); }                        \
    };                                                                          \

/// Construct a templated key generator that creates keys from static ints
#define mtkeygen(Name)                                                          \
    template<szt Index> struct Name                                             \
    {                                                                           \
        static mt_global(const Id, id, (sout() << #Name << "<" << Index << ">"));   \
        template<class Val> MtPair<Name, Val> operator=(Val&& val)              \
        { return MtPair<Name, Val>(forward<Val>(val)); }                        \
    };                                                                          \

/** \cond */
namespace priv
{
    template<class... Pairs> struct PairSeq {};
    
    template<class... Pairs> struct PairSeqGen_                             : PairSeqGen_<PairSeq<>, Pairs...> {};
    template<class... Seq, class Key, class Val, class... Pairs>
    struct PairSeqGen_<PairSeq<Seq...>, MtPair<Key, Val>, Pairs...>         : PairSeqGen_<PairSeq<Seq..., Val, Key>, Pairs...> {};
    template<class... Seq>
    struct PairSeqGen_<PairSeq<Seq...>>                                     { typedef PairSeq<Seq...> type; };
    
    /// Generate a pair type sequence <Val1,Key1, Val2,Key2, ...> from a series of pairs MtPair<Key, Val>
    template<class... Pairs> using PairSeqGen                               = typename PairSeqGen_<Pairs...>::type;
    
    template<class... Seq, class... Pairs>
    MtMap<Seq...> mtmap_(PairSeq<Seq...>, Pairs&&... pairs)                 { return MtMap<Seq...>(mt::tag<0>(), forward<Pairs>(pairs)...); }
}
/** \endcond */

/// Construct a map from `(key() = value)` pairs
template<class... Pairs>
auto mtmap(Pairs&&... pairs) ->
    decltype(priv::mtmap_(priv::PairSeqGen<Pairs...>(), forward<Pairs>(pairs)...))
{ return priv::mtmap_(priv::PairSeqGen<Pairs...>(), forward<Pairs>(pairs)...); }

/// Bidirectional iterator over map key/value pairs
/**
  * Every iteration step is a new type of iterator and pair, so this iterator can't be used normally.
  * This iterator should be used in a recursive templated function.
  */ 
template<class Head, class Elem = Head>
class MtMapIter
{
    friend class MtMapIter;

    typedef typename Elem::Key Key;
    typedef typename Elem::Val Val;
    typedef typename Head::Super::template findElem<Key>::Next Next;
    typedef typename Head::Super::template findElem<Key>::Prev Prev;

public:
    // Propogate const to pair value
    typedef MtPair<Key, decltype(declval<Head>().get(Key()))> Pair;

    MtMapIter(Head& head)                                           : _head(&head), pair(_head->get(Key())) {}

    typedef MtMapIter<Head, Next> NextIter;
    NextIter operator++()                                           { return NextIter(*_head); }
    typedef MtMapIter<Head, Prev> PrevIter;
    PrevIter operator--()                                           { return PrevIter(*_head); }

    template<class Iter> bool operator==(const Iter& rhs) const     { return _head == rhs._head && std::is_same<typename Pair::Key, typename Iter::Pair::Key>::value; }
    template<class Iter> bool operator!=(const Iter& rhs) const     { return !operator==(rhs); }

    const Pair& operator*() const                                   { return pair; }
    Pair& operator*()                                               { return pair; }
    const Pair* operator->() const                                  { return &pair; }
    Pair* operator->()                                              { return &pair; }

private:
    Head* _head;
    Pair pair;
};


/** \cond */
namespace priv
{
    /// If visitor can't accept key/value pair, skip the pair
    template<class Iter, class Func, typename std::enable_if<mt::isCallable<Func, typename Iter::Pair::Key, typename Iter::Pair::Val&>::value, int>::type=0>
    void for_each_mtmap_call(Iter& it, Func&& func)                { func(it->key, it->val); }
    template<class Iter, class Func, typename mt::disable_if<mt::isCallable<Func, typename Iter::Pair::Key, typename Iter::Pair::Val&>::value, int>::type=0>
    void for_each_mtmap_call(Iter&, Func&&) {}
}
/// Stop iteration when end is reached
template<class Iter1, class Iter2, class Func>
auto for_each_mtmap(Iter1, Iter2, Func&&) ->
    typename mt::disable_if<!std::is_same<typename Iter1::Pair::Key, typename Iter2::Pair::Key>::value>::type {}
/** \endcond */

/// Iterate over map calling functor (visitor) for each key/value pair
template<class Iter1, class Iter2, class Func>
auto for_each_mtmap(Iter1 itBegin, Iter2 itEnd, Func&& func) ->
    typename std::enable_if<!std::is_same<typename Iter1::Pair::Key, typename Iter2::Pair::Key>::value>::type
{
    priv::for_each_mtmap_call(itBegin, func);
    for_each_mtmap(++itBegin, itEnd, forward<Func>(func));
}


/** \cond */
namespace priv
{
    /// Check if value can be omitted in construction
    template<class Val> struct isOptional                           : std::false_type {};
    template<class Val> struct isOptional<optional<Val>>            : std::true_type {};
    template<> struct isOptional<mt::Void>                          : std::true_type {};

    /// Functor to convert map to string
    struct MtMapToString
    {
        MtMapToString(ostream& os)                                  : os(os), count(0) {}
        template<class Key, class Val>
        void operator()(Key, const Val& val)
        {
            if (count++ > 0) os << ", ";
            os << Key::id() << ": " << val;
        }
        ostream& os;
        szt count;
    };
}
/** \endcond */

/// Common functions between map elem and the map tail specialization. Use through class MtMapElem.
template<class Subclass, class Key_, class Val_, class List_>
class MtMapCommon
{
    template<class, class, class, class> friend class MtMapCommon;
    template<class, class> friend class MtMapIter;

public:
    typedef Key_ Key;   ///< Key type
    typedef Val_ Val;   ///< Value type
    typedef List_ List; ///< Rest of map

private:
    static const bool isTail = std::is_same<List, mt::Void>::value;

    /// Private functions for map elem
    template<bool isTail, szt _=0>
    struct priv
    {
        /// We don't have this key, recurse towards tail
        template<class Key, class Prev>
        struct findElem                                             : List::Super::template findElem<Key, Subclass> {};

        /// Specialize for our key
        template<class Prev_>
        struct findElem<Key, Prev_>
        {
            typedef Subclass type;
            typedef Prev_ Prev;
            typedef List Next;
        };

        /// Recurse to tail
        template<szt Count> struct sizeR                            : List::Super::template sizeR<Count+1> {};
    };

    /// Private functions for tail
    template<szt _>
    struct priv<true,_>
    {
        // Fallback for any key
        template<class Key, class Prev_>
        struct findElem
        {
            typedef Subclass type;
            typedef Prev_ Prev;
            typedef Subclass Next;
        };

        /// End recursion
        template<szt Count> struct sizeR                            : mt::Value<szt, Count> {};
    };

    /// Find map list element with key at compile-time.  Also returns prev/next elements in list at key.
    template<class Key, class Prev = Subclass>
    struct findElem                                                 : priv<isTail>::template findElem<Key, Prev> {};

    /// Recursive size counter, size is at tail
    template<szt Count>
    struct sizeR                                                    : priv<isTail>::template sizeR<Count> {};
    
public:
    /// Check if key exists at compile-time
    template<class Key> struct hasKey_                              : mt::Value<bool, !std::is_same<typename findElem<Key>::type, MtMapTail>::value> {};
    /// Check if has key
    template<class Key> bool hasKey(Key) const                      { return hasKey_<Key>::value; }

    /// Result type of get()
    template<class Key> using getResult                             = typename findElem<Key>::type::Val;

    /// Result type of begin()
    typedef MtMapIter<Subclass> beginResult;
    typedef MtMapIter<const Subclass> beginResult_const;
    
    /// Get beginning of an iterator over keys and values of this map
    beginResult         begin()                                     { return beginResult(subc()); }
    beginResult_const   begin() const                               { return beginResult_const(subc()); }
    
    /// Result type of end()
    typedef MtMapIter<Subclass, MtMapTail> endResult;
    typedef MtMapIter<const Subclass, MtMapTail> endResult_const;
        
    /// Get end of an iterator over keys and values of this map
    endResult       end()                                           { return endResult(subc()); }
    endResult_const end() const                                     { return endResult_const(subc()); }

    /// Result type of iter()
    template<class Key> using iterResult                            = MtMapIter<Subclass, typename findElem<Key>::type>;
    template<class Key> using iterResult_const                      = MtMapIter<const Subclass, typename findElem<Key>::type>;
    
    /// Get iterator to element by key
    template<class Key>
    iterResult<Key>         iter(Key)                               { return iterResult<Key>(subc()); }
    template<class Key>
    iterResult_const<Key>   iter(Key) const                         { return iterResult_const<Key>(subc()); }
    
private:
    template<class... Pairs> struct insertResult_                   { typedef Subclass type; };
    template<class Val, class Key, class... Pairs>
    struct insertResult_<Val, Key, Pairs...>
    {
        static_assert(!hasKey_<Key>::value, "Insert failed. Key already exists.");
        typedef MtMapElem<Key, Val, typename insertResult_<Pairs...>::type> type;
    };
    
    template<class PairSeq, class... Pairs> struct insertResult_seq;
    template<class... Seq, class... Pairs>
    struct insertResult_seq<honey::priv::PairSeq<Seq...>, Pairs...> { typedef typename insertResult_<Seq...>::type type; };
    
public:
    /// Result type of insert(). New pairs are inserted at the front.
    template<class... Pairs> using insertResult                     = typename insertResult_<Pairs...>::type;
    
    /// Insert pairs of the form `(key() = value)` into the map
    template<class... Pairs>
    typename insertResult_seq<honey::priv::PairSeqGen<Pairs...>, Pairs...>::type
        insert(Pairs&&... pairs) const                              { return typename insertResult_seq<honey::priv::PairSeqGen<Pairs...>, Pairs...>::type(mt::tag<1>(), subc(), forward<Pairs>(pairs)...); }
    
private:
    template<bool isTail, class... Keys>
    struct eraseResult_
    {
        typedef typename std::conditional<
            mt::typeIndex<Key, Keys...>::value >= 0,
            typename List::Super::template eraseResult_<List::Super::isTail, Keys...>::type,
            MtMapElem<Key, Val, typename List::Super::template eraseResult_<List::Super::isTail, Keys...>::type>
        >::type type;
    };
    template<class... Keys>
    struct eraseResult_<true, Keys...>                              { typedef MtMapTail type; };
    
    template<class _=void> struct clearResult_                      { typedef MtMapTail type; };
    
public:
    /// Result type of erase().  Reconstructs type without matching keys.
    template<class... Keys> using eraseResult                       = typename eraseResult_<isTail, Keys...>::type;
    
    /// Erase keys from the map
    template<class... Keys>
    eraseResult<Keys...> erase(Keys...) const                       { return eraseResult<Keys...>(subc()); }
    
    /// Result type of clear()
    typedef typename clearResult_<>::type clearResult;
    /// Clear map of all keys
    clearResult clear()                                             { return clearResult(); }

    /// Get size of map at compile-time
    struct size_                                                    : sizeR<0> {};
    /// Get size of map
    szt size() const                                                { return size_::value; };

    /// Check if empty at compile-time
    struct empty_                                                   : mt::Value<bool, isTail> {};
    /// Check if empty
    bool empty() const                                              { return empty_::value; }
    
    friend ostream& operator<<(ostream& os, const Subclass& map)
    { os << "{ "; for_each_mtmap(map.begin(), map.end(), honey::priv::MtMapToString(os)); os << " }"; return os; }

private:
    /// Get the subclass that inherited from this base class
    const Subclass& subc() const                                    { return static_cast<const Subclass&>(*this); }
    Subclass& subc()                                                { return static_cast<Subclass&>(*this); }
};

/// Map element in recursive list
/**
  * \see MtMapCommon for the rest of the members.
  * \see \ref MtMap for more info and examples.
  */  
template<class Key_, class Val_, class List_>
class MtMapElem : public MtMapCommon<MtMapElem<Key_,Val_,List_>, Key_, Val_, List_>, public List_
{
    template<class, class, class, class> friend class MtMapCommon;
    template<class, class, class> friend class MtMapElem;
    template<class, class> friend class MtMapIter;
    typedef MtMapCommon<MtMapElem, Key_, Val_, List_> Super;

public:
    using typename Super::Key;
    using typename Super::Val;
    using typename Super::List;
    template<class Key> using hasKey_                               = typename Super::template hasKey_<Key>;
    using Super::hasKey;
    template<class Key> using getResult                             = typename Super::template getResult<Key>;
    using typename Super::beginResult;
    using typename Super::beginResult_const;
    using Super::begin;
    using typename Super::endResult;
    using typename Super::endResult_const;
    using Super::end;
    template<class Key> using iterResult                            = typename Super::template iterResult<Key>;
    template<class Key> using iterResult_const                      = typename Super::template iterResult_const<Key>;
    using Super::iter;
    template<class... Pairs> using insertResult                     = typename Super::template insertResult<Pairs...>;
    using Super::insert;
    template<class... Keys> using eraseResult                       = typename Super::template eraseResult<Keys...>;
    using Super::erase;
    using typename Super::clearResult;
    using Super::clear;
    using typename Super::size_;
    using Super::size;
    using typename Super::empty_;
    using Super::empty;

    MtMapElem()                                                     { static_assert(honey::priv::isOptional<Val>::value, "Key not optional. Must provide key to constructor."); }
    
    /// Ctor, pairs must be in correct order
    template<class Pair, class... Pairs>
    MtMapElem(mt::tag<0>, Pair&& pair, Pairs&&... pairs) :
        List(mt::tag<0>(), forward<Pairs>(pairs)...),
        _val(forward<typename Pair::Val>(pair.val))
    { static_assert(std::is_same<Key,typename Pair::Key>::value, "Ctor failed. Key mismatch. Wrong init order."); }
    
    /// Copy/Move any map type. Init with matching key in other map, recurse to tail.
    template<class Map>
    MtMapElem(Map&& map)                                            : List(forward<Map>(map)), _val(priv<Map>::init(map.get(Key()))) {}

    /// Copy/Move-assign any map type. Assign to matching key in other map, recurse to tail.
    template<class Map>
    MtMapElem& operator=(Map&& rhs)                                 { priv<Map>::assign(_val, rhs.get(Key())); List::operator=(forward<Map>(rhs)); return *this; }

    using List::operator[];
    /// Get value at key
    const Val& operator[](Key) const                                { return _val; }
    Val& operator[](Key)                                            { return _val; }

    using List::get;
    /// Get value at key
    const Val& get(Key) const                                       { return _val; }
    Val& get(Key)                                                   { return _val; }

    using List::set;
    /// Set value at key from the pair `(key() = value)`.  Returns false if the key isn't found.
    template<class Val>
    bool set(MtPair<Key, Val>&& pair)                               { _val = forward<Val>(pair.val); return true; }
    
    /// Set any uninitialized optional values to the defaults provided.  A default for a key must be a functor that returns the value, so that the value ctor can be omitted if the key is already set.
    template<class Map>
    void setDefaults(Map&& defaults)                                { priv<Map>::setDefault(_val, defaults.get(Key())); List::setDefaults(forward<Map>(defaults)); }

private:
    template<class Map>
    struct priv
    {
        typedef typename mt::removeRef<Map>::type Map_;

        static const bool movable = !mt::isLref<Map>::value && !mt::isLref<typename Map_::template getResult<Key>>::value;
        static const bool optional = honey::priv::isOptional<Val>::value;

        template<class T>
        static auto init(T&& val) -> typename std::enable_if<mt::True<T>::value && movable, decltype(move(val))>::type  { return move(val); }
        template<class T>
        static auto init(T&& val) -> typename mt::disable_if<mt::True<T>::value && movable, T&>::type                   { return val; }
        static Val init(mt::Void)
        {
            static_assert(optional, "Key not optional. Must provide key to constructor.");
            return Val();
        }

        template<class T>
        static auto assign(Val& lhs, T&& rhs) -> typename std::enable_if<mt::True<T>::value && movable>::type           { lhs = move(rhs); }
        template<class T>
        static auto assign(Val& lhs, T&& rhs) -> typename mt::disable_if<mt::True<T>::value && movable>::type           { lhs = rhs; }
        static void assign(Val&, mt::Void){}

        template<class T>
        static auto setDefault(Val& lhs, T&& rhs) -> typename std::enable_if<mt::True<T>::value && optional>::type      { if (!lhs) lhs = rhs(); }
        template<class T>
        static auto setDefault(Val&, T&&) -> typename mt::disable_if<mt::True<T>::value && optional>::type {}
        static void setDefault(Val&, mt::Void) {}
    };
    
    /// Insert ctor, after new pairs are processed the source map is copied with copy ctor
    template<class Map, class Pair, class... Pairs>
    MtMapElem(mt::tag<1>, const Map& map, Pair&& pair, Pairs&&... pairs) :
        List(mt::tag<1>(), map, forward<Pairs>(pairs)...),
        _val(forward<typename Pair::Val>(pair.val)) {}

    template<class Map, class Pair>
    MtMapElem(mt::tag<1>, const Map& map, Pair&& pair) :
        List(map),
        _val(forward<typename Pair::Val>(pair.val)) {}
    
    /// Stored value at this key
    Val _val;
};

/** \cond */
/// Map tail, handles unknown keys and end of recursion
template<>
class MtMapElem<mt::Void, mt::Void, mt::Void> : public MtMapCommon<MtMapTail, mt::Void, mt::Void, mt::Void>
{
    template<class, class, class, class> friend class MtMapCommon;
    template<class, class, class> friend class MtMapElem;
    template<class, class> friend class MtMapIter;
    typedef MtMapCommon<MtMapElem, Key, Val, List> Super;

public:
    MtMapElem() {}
    /// Copy/Move-any ctor, end recursion
    template<class Map> MtMapElem(Map&&) {}
    /// Copy/Move-assign-any, end recursion
    template<class Map> MtMapElem& operator=(Map&&)                 { return *this; }
    /// Fallback
    template<class Key> Val operator[](Key) const                   { return Val(); }
    template<class Key> Val operator[](Key)                         { return Val(); }
    /// Fallback
    template<class Key> Val get(Key) const                          { return Val(); }
    template<class Key> Val get(Key)                                { return Val(); }
    /// Fallback
    template<class Key, class Val>
    bool set(MtPair<Key, Val>&&)                                    { return false; }
    /// End recursion
    template<class Map> void setDefaults(Map&&) {}
    
private:
    /// Ctor, end recursion
    MtMapElem(mt::tag<0>) {}
};
/** \endcond */

/// @}

}
