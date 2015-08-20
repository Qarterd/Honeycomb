// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Numeral.h"

namespace honey
{

/// Methods that create and operate on ranges
/**
  * \defgroup Range     Range Util
  */
/// @{

/// Max args for range related variable argument functions
#define RANGE_ARG_MAX 3

namespace mt
{
    /// Check if type is a range or a reference to one. A range is a type where std::begin(range) is a valid call.
    template<class T>
    class isRange
    {
        template<class T_> static auto              test(void*) -> decltype(begin(declval<T_>()), std::true_type());
        template<class T_> static std::false_type   test(...);
    public:
        static const bool value = isTrue<decltype(test<T>(nullptr))>::value;
    };
    
    /// Get range iterator begin type. \see elemOf
    template<class Range> struct iterOf                             { typedef typename std::decay<decltype(honey::begin(declval<Range>()))>::type type; };
    /// iterOf for values
    #define mt_iterOf(Range)                                        typename std::decay<decltype(honey::begin(Range))>::type
    
    /// Get range iterator end type. \see elemOf
    template<class Range> struct iter_endOf                         { typedef typename std::decay<decltype(honey::end(declval<Range>()))>::type type; };
    /// iter_endOf for values
    #define mt_iter_endOf(Range)                                    typename std::decay<decltype(honey::end(Range))>::type
    
    /// Get range element type. \see iterOf
    template<class Range> struct elemOf                             { typedef typename mt::removeRef<decltype(*honey::begin(declval<Range>()))>::type type; };
    /// elemOf for values
    #define mt_elemOf(Range)                                        typename mt::removeRef<decltype(*honey::begin(Range))>::type
    
    /** \cond */
    namespace priv
    {
        template<class Result, class T>
        Result valAt(int cur, int end, T&& t)                       { assert(cur == end, "Index out of pack range"); return t; }
        template<class Result, class T, class... Ts>
        Result valAt(int cur, int end, T&& t, Ts&&... ts)           { return cur == end ? t : valAt<Result>(cur+1, end, forward<Ts>(ts)...); }
        
        template<class Val, class T>
        int valIndex(int cur, Val&& val, T&& t)                     { return val == t ? cur : -1; }
        template<class Val, class T, class... Ts>
        int valIndex(int cur, Val&& val, T&& t, Ts&&... ts)         { return val == t ? cur : valIndex(cur+1, forward<Val>(val), forward<Ts>(ts)...); }
    }
    /** \endcond */
    
    /// Get value at index of parameter pack. All types must be convertible to `Result`, which defaults to the first type in `Ts`.
    template<class Result = void, class... Ts>
    typename std::conditional<std::is_same<Result, void>::value, mt::typeAt<0, Ts...>, Result>::type
        valAt(int i, Ts&&... ts)
    {
        typedef typename std::conditional<std::is_same<Result, void>::value, mt::typeAt<0, Ts...>, Result>::type Result_;
        return priv::valAt<Result_>(0, i, forward<Ts>(ts)...);
    }
    
    /// Get index of first matching value in parameter pack, returns -1 if not found
    template<class Val, class... Ts>
    int valIndex(Val&& val, Ts&&... ts)                             { return priv::valIndex(0, forward<Val>(val), forward<Ts>(ts)...); }
}

/// Convert a sequence to a forward iterator. Overload for iterator type. Returns the iterator itself.
/**
  * A sequence is defined as anything convertible to a forward iterator (eg. an iterator, a range).
  */
template<class Iter>
auto seqToIter(Iter&& seq) -> typename std::enable_if<mt::isIterator<Iter>::value, Iter&&>::type
                                                                    { return forward<Iter>(seq); }
/// Convert a sequence to a forward iterator. Overload for range type. Returns the range's begin iterator.
template<class Range>
auto seqToIter(Range&& seq) -> typename std::enable_if<mt::isRange<Range>::value, mt_iterOf(seq)>::type
                                                                    { return begin(seq); }

/// Iterator range. See range(Iter1&&, Iter2&&) to create.
template<class T1_, class T2_>
class Range_
{
    template<class T1, class T2> friend class Range_;
public:
    typedef typename std::decay<T1_>::type T1;
    typedef typename std::decay<T2_>::type T2;

    Range_() = default;
    template<class T1, class T2> Range_(T1&& first, T2&& last)                  : _first(forward<T1>(first)), _last(forward<T2>(last)) {}
    template<class T1, class T2> Range_(const Range_<T1,T2>& rhs)               { operator=(rhs); }
    template<class T1, class T2> Range_(Range_<T1,T2>&& rhs)                    { operator=(move(rhs)); }

    template<class T1, class T2> Range_& operator=(const Range_<T1,T2>& rhs)    { _first = rhs._first; _last = rhs._last; return *this; }
    template<class T1, class T2> Range_& operator=(Range_<T1,T2>&& rhs)         { _first = move(rhs._first); _last = move(rhs._last); return *this; }

    const T1& begin() const                                                     { return _first; }
    T1& begin()                                                                 { return _first; }
    const T2& end() const                                                       { return _last; }
    T2& end()                                                                   { return _last; }

private:
    T1 _first;
    T2 _last;
};

/// Range from iterators [first, last)
template<class Iter1, class Iter2>
typename std::enable_if<mt::isIterator<Iter1>::value, Range_<Iter1,Iter2>>::type
    range(Iter1&& first, Iter2&& last)                              { return Range_<Iter1,Iter2>(forward<Iter1>(first), forward<Iter2>(last)); }

/// Range from iterator pair [first, second)
template<class T1, class T2>
Range_<T1,T2> range(pair<T1,T2>& p)                                 { return Range_<T1,T2>(p.first, p.second); }
template<class T1, class T2>
Range_<T1,T2> range(const pair<T1,T2>& p)                           { return Range_<T1,T2>(p.first, p.second); }

/// Range from iterator tuple [0, 1)
template<class T1, class T2>
Range_<T1,T2> range(tuple<T1,T2>& t)                                { return Range_<T1,T2>(get<0>(t), get<1>(t)); }
template<class T1, class T2>
Range_<T1,T2> range(const tuple<T1,T2>& t)                          { return Range_<T1,T2>(get<0>(t), get<1>(t)); }

/// Reverse a range.  Begin/End iterators must be bidirectional.
template<class Range>
auto reversed(Range&& range) -> Range_<std::reverse_iterator<mt_iterOf(range)>, std::reverse_iterator<mt_iter_endOf(range)>>
{
    return honey::range(std::reverse_iterator<mt_iterOf(range)>(end(range)), std::reverse_iterator<mt_iter_endOf(range)>(begin(range)));
}

//====================================================
/** \cond */
#define map(...) __map()
#define OutSeq void*
/** \endcond */
/// Transform a series of sequences into an output
/**
  * Call function for each element in the range and any sequences, `f(rangeElem, seq1Elem, seq2Elem...)`,
  * storing the results in the output sequence. Returns the output sequence.
  *
  * `map()` can be specialized through the function: `priv::map_impl<Range, OutSeq, Seqs...>::%func(...)`
  */
OutSeq&& map(Range&&, Seqs&&..., OutSeq&&, Func&&);
#undef map
#undef OutSeq
/** \cond */
#define PARAMT(It)      , class S##It
#define PARAMT_(It)     , class S##It##_
#define PARAMT_SPEC(It) , S##It##_
#define PARAM(It)       , S##It&& seq##It
#define SEQTOITER(It)   auto it##It = seqToIter(seq##It);
#define NEXT(It)        , ++it##It
#define ARG(It)         , *it##It
#define FORWARDT(It)    , typename std::decay<S##It>::type
#define FORWARD(It)     , forward<S##It>(seq##It)

namespace priv
{
    template<class Range, class OutSeq, class... Seqs> struct map_impl;
}

#define FUNC(It)                                                                                                    \
    namespace priv                                                                                                  \
    {                                                                                                               \
    template<class Range_, class OutSeq_ ITERATE_(1,It,PARAMT_)>                                                    \
    struct map_impl<Range_, OutSeq_ ITERATE_(1,It,PARAMT_SPEC)>                                                     \
    {                                                                                                               \
        template<class Range ITERATE_(1,It,PARAMT), class OutSeq, class Func>                                       \
        static OutSeq&& func(Range&& range ITERATE_(1,It,PARAM), OutSeq&& out, Func&& f)                            \
        {                                                                                                           \
            auto it = begin(range);                                                                                 \
            auto last = end(range);                                                                                 \
            ITERATE_(1,It,SEQTOITER)                                                                                \
            auto out_it = seqToIter(out);                                                                           \
            for (; it != last; ++it ITERATE_(1,It,NEXT), ++out_it)                                                  \
                *out_it = f(*it ITERATE_(1,It,ARG));                                                                \
            return forward<OutSeq>(out);                                                                            \
        }                                                                                                           \
    };                                                                                                              \
    }                                                                                                               \
                                                                                                                    \
    template<class Range ITERATE_(1,It,PARAMT), class OutSeq, class Func>                                           \
    OutSeq&& map(Range&& range ITERATE_(1,It,PARAM), OutSeq&& out, Func&& f)                                        \
    {                                                                                                               \
        return priv::map_impl<  typename std::decay<Range>::type, typename std::decay<OutSeq>::type                 \
                                ITERATE_(1,It,FORWARDT)>                                                            \
                                ::func( forward<Range>(range) ITERATE_(1,It,FORWARD),                               \
                                        forward<OutSeq>(out), forward<Func>(f));                                    \
    }                                                                                                               \

ITERATE(0, RANGE_ARG_MAX, FUNC)
#undef PARAMT
#undef PARAMT_
#undef PARAMT_SPEC
#undef PARAM
#undef SEQTOITER
#undef NEXT
#undef ARG
#undef FORWARDT
#undef FORWARD
#undef FUNC
/** \endcond */
//====================================================

//====================================================
/** \cond */
#define reduce(...) __reduce()
#define Accum void*
/** \endcond */
/// Accumulate a series of sequences into an output
/**
  * Call function for each element in the range and any sequences, `f(accum, rangeElem, seq1Elem, seq2Elem...)`,
  * forwarding the result of each call into the next as `accum`.  Returns the accumulated value.
  *
  * Reduce can be specialized through the function: `priv::reduce_impl<Range, Accum, Seqs...>::%func(...)`
  */
Accum reduce(Range&&, Seqs&&..., Accum&& initVal, Func&&);
#undef reduce
#undef Accum
/** \cond */
#define PARAMT(It)      , class S##It
#define PARAMT_(It)     , class S##It##_
#define PARAMT_SPEC(It) , S##It##_
#define PARAM(It)       , S##It&& seq##It
#define SEQTOITER(It)   auto it##It = seqToIter(seq##It);
#define NEXT(It)        , ++it##It
#define ARG(It)         , *it##It
#define FORWARDT(It)    , typename std::decay<S##It>::type
#define FORWARD(It)     , forward<S##It>(seq##It)

namespace priv
{
    template<class Range, class Accum, class... Seqs> struct reduce_impl;
}

#define FUNC(It)                                                                                                    \
    namespace priv                                                                                                  \
    {                                                                                                               \
    template<class Range_, class Accum_ ITERATE_(1,It,PARAMT_)>                                                     \
    struct reduce_impl<Range_, Accum_ ITERATE_(1,It,PARAMT_SPEC)>                                                   \
    {                                                                                                               \
        template<class Range ITERATE_(1,It,PARAMT), class Accum, class Func>                                        \
        static Accum_ func(Range&& range ITERATE_(1,It,PARAM), Accum&& initVal, Func&& f)                           \
        {                                                                                                           \
            Accum_ a(forward<Accum>(initVal));                                                                      \
            auto it = begin(range);                                                                                 \
            auto last = end(range);                                                                                 \
            ITERATE_(1,It,SEQTOITER)                                                                                \
            for (; it != last; ++it ITERATE_(1,It,NEXT))                                                            \
                a = f(a, *it ITERATE_(1,It,ARG));                                                                   \
            return a;                                                                                               \
        }                                                                                                           \
    };                                                                                                              \
    }                                                                                                               \
                                                                                                                    \
    template<   class Range ITERATE_(1,It,PARAMT), class Accum, class Func,                                         \
                class Accum_ = typename std::decay<Accum>::type>                                                    \
    Accum_ reduce(Range&& range ITERATE_(1,It,PARAM), Accum&& initVal, Func&& f)                                    \
    {                                                                                                               \
        return priv::reduce_impl<typename std::decay<Range>::type, Accum_ ITERATE_(1,It,FORWARDT)>                  \
                                    ::func( forward<Range>(range) ITERATE_(1,It,FORWARD),                           \
                                            forward<Accum>(initVal), forward<Func>(f));                             \
    }                                                                                                               \

ITERATE(0, RANGE_ARG_MAX, FUNC)
#undef PARAMT
#undef PARAMT_
#undef PARAMT_SPEC
#undef PARAM
#undef SEQTOITER
#undef NEXT
#undef ARG
#undef FORWARDT
#undef FORWARD
#undef FUNC
/** \endcond */
//====================================================

//====================================================
/** \cond */
#define find(...) __find()
#define Iter void*
/** \endcond */
/// Find an element in a series of sequences
/**
  * Call predicate function for each element in the range and any sequences, `pred(rangeElem, seq1Elem, seq2Elem...)`,
  * returning an iterator to the first element for which the predicate returns true.
  * Returns range end if the predicate is false for all elements.
  */
Iter find(Range&&, Seqs&&..., Func&& pred);
#undef find
#undef Iter
/** \cond */
#define PARAMT(It)      , class S##It
#define PARAM(It)       , S##It&& seq##It
#define SEQTOITER(It)   auto it##It = seqToIter(seq##It);
#define NEXT(It)        , ++it##It
#define ARG(It)         , *it##It

#define FUNC(It)                                                                                                    \
    template<class Range ITERATE_(1,It,PARAMT), class Func>                                                         \
    auto find(Range&& range ITERATE_(1,It,PARAM), Func&& pred) -> mt_iterOf(range)                                  \
    {                                                                                                               \
        auto it = begin(range);                                                                                     \
        auto last = end(range);                                                                                     \
        ITERATE_(1,It,SEQTOITER)                                                                                    \
        for (; it != last; ++it ITERATE_(1,It,NEXT))                                                                \
            if (pred(*it ITERATE_(1,It,ARG))) break;                                                                \
        return it;                                                                                                  \
    }                                                                                                               \

ITERATE(0, RANGE_ARG_MAX, FUNC)
#undef PARAMT
#undef PARAM
#undef SEQTOITER
#undef NEXT
#undef ARG
#undef FUNC
/** \endcond */
//====================================================

//====================================================
/** \cond */
#define filter(...) __filter()
#define FilterIter int*
/** \endcond */
/// Filter a range by ignoring undesired elements
/**
  * Behaviour is the same as repeatedly calling find() passing in the returned iterator until the end is reached.
  */
Range_<FilterIter, FilterIter> filter(Range&&, Seqs&&..., Func&& pred);
#undef filter
#undef FilterIter
/** \cond */
#define PARAMT(It)              , class S##It
#define PARAM(It)               , S##It&& seq##It
#define PARAM_REF(It)           , const S##It& seq##It
#define SEQTOITER_PARAMT(It)    , decltype(seqToIter(seq##It))
#define SEQTOITER(It)           , seqToIter(seq##It)
#define MEMBER(It)              S##It _seq##It; 
#define MEMBER_INIT(It)         , _seq##It(seq##It)
#define NEXT(It)                , ++_seq##It
#define ARG(It)                 , *_seq##It

#define FUNC(It)                                                                                                        \
    template<class Range ITERATE_(1,It,PARAMT), class Func>                                                             \
    class FilterIter##It                                                                                                \
    {                                                                                                                   \
    public:                                                                                                             \
        typedef typename mt::iterOf<Range>::type                        Iter;                                           \
        typedef typename mt::iter_endOf<Range>::type                    IterEnd;                                        \
        typedef std::forward_iterator_tag                               iterator_category;                              \
        typedef typename std::iterator_traits<Iter>::value_type         value_type;                                     \
        typedef typename std::iterator_traits<Iter>::difference_type    difference_type;                                \
        typedef typename std::iterator_traits<Iter>::pointer            pointer;                                        \
        typedef typename std::iterator_traits<Iter>::reference          reference;                                      \
                                                                                                                        \
        FilterIter##It(const IterEnd& end, const Func& pred)            : _it(end), _itEnd(end), _pred(pred) {}         \
                                                                                                                        \
        FilterIter##It(const Iter& begin, const IterEnd& end ITERATE_(1,It,PARAM_REF), const Func& pred) :              \
            _it(begin), _itEnd(end) ITERATE_(1,It,MEMBER_INIT), _pred(pred) { next(); }                                 \
                                                                                                                        \
        FilterIter##It& operator++()                                                                                    \
        {                                                                                                               \
            assert(_it != _itEnd);                                                                                      \
            ++_it ITERATE_(1,It,NEXT);                                                                                  \
            next();                                                                                                     \
            return *this;                                                                                               \
        }                                                                                                               \
        FilterIter##It operator++(int)                                  { auto tmp = *this; ++*this; return tmp; }      \
                                                                                                                        \
        bool operator==(const FilterIter##It& rhs) const                { return _it == rhs._it; }                      \
        bool operator!=(const FilterIter##It& rhs) const                { return !operator==(rhs); }                    \
                                                                                                                        \
        reference operator*() const                                     { return *_it; }                                \
        pointer operator->() const                                      { return _it.operator->(); }                    \
        operator Iter() const                                           { return _it; }                                 \
                                                                                                                        \
    private:                                                                                                            \
        void next()                                                                                                     \
        {                                                                                                               \
            for (; _it != _itEnd; ++_it ITERATE_(1,It,NEXT))                                                            \
                if (_pred(*_it ITERATE_(1,It,ARG))) break;                                                              \
        }                                                                                                               \
                                                                                                                        \
        Iter _it;                                                                                                       \
        IterEnd _itEnd;                                                                                                 \
        ITERATE_(1,It,MEMBER)                                                                                           \
        Func _pred;                                                                                                     \
    };                                                                                                                  \
                                                                                                                        \
    template<class Range ITERATE_(1,It,PARAMT), class Func>                                                             \
    auto filter(Range&& range ITERATE_(1,It,PARAM), Func&& pred) ->                                                     \
        Range_< FilterIter##It<Range ITERATE_(1,It,SEQTOITER_PARAMT), Func>,                                            \
                FilterIter##It<Range ITERATE_(1,It,SEQTOITER_PARAMT), Func>>                                            \
    {                                                                                                                   \
        typedef FilterIter##It<Range ITERATE_(1,It,SEQTOITER_PARAMT), Func> FilterIter;                                 \
        return honey::range(FilterIter(begin(range), end(range) ITERATE_(1,It,SEQTOITER), pred),                        \
                            FilterIter(end(range), pred));                                                              \
    }                                                                                                                   \

ITERATE(0, RANGE_ARG_MAX, FUNC)
#undef PARAMT
#undef PARAM
#undef PARAM_REF
#undef SEQTOITER_PARAMT
#undef SEQTOITER
#undef MEMBER
#undef MEMBER_INIT
#undef NEXT
#undef ARG
#undef FUNC
/** \endcond */
//====================================================

/// Count number of elements in range
template<class Range>
szt countOf(Range&& range)                                          { return reduce(range, szt(0), [](szt a, auto&) { return ++a; }); }

/// Delete all elements in range
template<class Range>
void deleteRange(Range&& range)                                     { for (auto& e : range) delete_(e); }

/// Delete all elements in range using allocator
template<class Range, class Alloc>
void deleteRange(Range&& range, Alloc&& a)                          { for (auto& e : range) delete_(e, a); }


/// Incremental integer iterator (step size = 1). See range(int, int) to create.
template<class T>
class IntIter
{
public:
    typedef std::random_access_iterator_tag                         iterator_category;
    typedef T                                                       value_type;
    typedef T                                                       difference_type;
    typedef const T*                                                pointer;
    typedef const T&                                                reference;

    IntIter() = default;
    IntIter(T i)                                                    : _i(i) {}

    IntIter& operator++()                                           { ++_i; return *this; }
    IntIter& operator--()                                           { --_i; return *this; }
    IntIter operator++(int)                                         { auto tmp = *this; ++*this; return tmp; }
    IntIter operator--(int)                                         { auto tmp = *this; --*this; return tmp; }
    IntIter& operator+=(difference_type rhs)                        { _i += rhs; return *this; }
    IntIter& operator-=(difference_type rhs)                        { _i -= rhs; return *this; }
    IntIter operator+(difference_type rhs) const                    { auto tmp = *this; tmp+=rhs; return tmp; }
    IntIter operator-(difference_type rhs) const                    { auto tmp = *this; tmp-=rhs; return tmp; }
    difference_type operator-(const IntIter& rhs) const             { return _i - rhs._i; }

    bool operator==(const IntIter& rhs) const                       { return _i == rhs._i; }
    bool operator!=(const IntIter& rhs) const                       { return _i != rhs._i; }
    bool operator< (const IntIter& rhs) const                       { return _i <  rhs._i; }
    bool operator> (const IntIter& rhs) const                       { return _i >  rhs._i; }
    bool operator<=(const IntIter& rhs) const                       { return _i <= rhs._i; }
    bool operator>=(const IntIter& rhs) const                       { return _i >= rhs._i; }

    reference operator*() const                                     { return _i; }
    operator T() const                                              { return _i; }

private:
    T _i;
};

/// Create a range that increments through the integral range [begin,end)
template<class Int, class Int2, class Int_ = typename std::common_type<Int,Int2>::type>
typename std::enable_if<std::is_integral<Int_>::value, Range_<IntIter<Int_>, IntIter<Int_>>>::type
    range(Int begin, Int2 end)
{
    // Make sure begin comes before end
    return range(IntIter<Int_>(begin), IntIter<Int_>(end < begin ? begin : end));
}

/// Create a range that increments through the integral range [0,end)
template<class Int>
typename std::enable_if<std::is_integral<Int>::value, Range_<IntIter<Int>, IntIter<Int>>>::type
    range(Int end)                                                  { return range(Int(0), end); }


/// Integer iterator with step size. See range(int, int, int) to create.
template<class T>
class IntStepIter
{
public:
    typedef std::random_access_iterator_tag                         iterator_category;
    typedef T                                                       value_type;
    typedef T                                                       difference_type;
    typedef const T*                                                pointer;
    typedef const T&                                                reference;

    IntStepIter() = default;
    IntStepIter(T i, T step)                                        : _i(i), _step(step) {}

    IntStepIter& operator++()                                       { _i += _step; return *this; }
    IntStepIter& operator--()                                       { _i -= _step; return *this; }
    IntStepIter operator++(int)                                     { auto tmp = *this; ++*this; return tmp; }
    IntStepIter operator--(int)                                     { auto tmp = *this; --*this; return tmp; }
    IntStepIter& operator+=(difference_type rhs)                    { _i += rhs*_step; return *this; }
    IntStepIter& operator-=(difference_type rhs)                    { _i -= rhs*_step; return *this; }
    IntStepIter operator+(difference_type rhs) const                { auto tmp = *this; tmp+=rhs*_step; return tmp; }
    IntStepIter operator-(difference_type rhs) const                { auto tmp = *this; tmp-=rhs*_step; return tmp; }
    difference_type operator-(const IntStepIter& rhs) const         { return _i - rhs._i; }

    bool operator==(const IntStepIter& rhs) const                   { return _i == rhs._i; }
    bool operator!=(const IntStepIter& rhs) const                   { return _i != rhs._i; }
    bool operator< (const IntStepIter& rhs) const                   { return _i <  rhs._i; }
    bool operator> (const IntStepIter& rhs) const                   { return _i >  rhs._i; }
    bool operator<=(const IntStepIter& rhs) const                   { return _i <= rhs._i; }
    bool operator>=(const IntStepIter& rhs) const                   { return _i >= rhs._i; }

    reference operator*() const                                     { return _i; }
    operator T() const                                              { return _i; }

private:
    T _i;
    T _step;
};

/// Create a range that steps through the integral range [begin,end)
template<class Int, class Int2, class Int3, class Int_ = typename std::common_type<Int,Int2,Int3>::type>
typename std::enable_if<std::is_integral<Int_>::value, Range_<IntStepIter<Int_>, IntStepIter<Int_>>>::type
    range(Int begin, Int2 end, Int3 step)
{
    assert(step != 0);
    // Make sure begin comes before end
    Int_ end_ = step > 0 ? (end < begin ? begin : end) : (end > begin ? begin : end);
    Int_ dif = end_ - begin;
    return range(IntStepIter<Int_>(begin, step), IntStepIter<Int_>(begin + (dif/step + (dif%step!=0))*step, step));
}


/// Real number iterator. See range(Real, Real, Real) to create.
template<class T>
class RealIter
{
    typedef typename Numeral<T>::Int Int;
public:
    typedef std::random_access_iterator_tag                         iterator_category;
    typedef T                                                       value_type;
    typedef Int                                                     difference_type;
    typedef T*                                                      pointer;
    typedef T                                                       reference;

    RealIter() = default;
    RealIter(T begin, T step, Int i)                                : _begin(begin), _step(step), _i(i) {}

    RealIter& operator++()                                          { ++_i; return *this; }
    RealIter& operator--()                                          { --_i; return *this; }
    RealIter operator++(int)                                        { auto tmp = *this; ++*this; return tmp; }
    RealIter operator--(int)                                        { auto tmp = *this; --*this; return tmp; }
    RealIter& operator+=(difference_type rhs)                       { _i += rhs; return *this; }
    RealIter& operator-=(difference_type rhs)                       { _i -= rhs; return *this; }
    RealIter operator+(difference_type rhs) const                   { auto tmp = *this; tmp+=rhs; return tmp; }
    RealIter operator-(difference_type rhs) const                   { auto tmp = *this; tmp-=rhs; return tmp; }
    difference_type operator-(const RealIter& rhs) const            { return _i - rhs._i; }

    bool operator==(const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i == rhs._i; }
    bool operator!=(const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i != rhs._i; }
    bool operator< (const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i <  rhs._i; }
    bool operator> (const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i >  rhs._i; }
    bool operator<=(const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i <= rhs._i; }
    bool operator>=(const RealIter& rhs) const                      { assert(_begin == rhs._begin && _step == rhs._step); return _i >= rhs._i; }

    reference operator*() const                                     { return _begin + _i*_step; }
    operator T() const                                              { return _begin + _i*_step; }

private:
    T _begin;
    T _step;
    Int _i;
};

/// Create a range that steps through the real number range [begin,end)
template<class Real, class Real2, class Real3, class Real_ = typename std::common_type<Real,Real2,Real3>::type>
typename std::enable_if<std::is_floating_point<Real_>::value, Range_<RealIter<Real_>, RealIter<Real_>>>::type
    range(Real begin, Real2 end, Real3 step = 1)
{
    assert(step != 0);
    // Make sure begin comes before end
    Real_ end_ = step > 0 ? (end < begin ? begin : end) : (end > begin ? begin : end);
    typedef typename Numeral<Real_>::Real_ Real__;
    return range(RealIter<Real_>(begin, step, 0), RealIter<Real_>(begin, step, Real__::ceil((end_-begin)/step)));
}


/// Wrapper around an iterator with tuple value type. When dereferenced returns `I`'th element.
template<class Iter, szt I, class IterCategory = typename Iter::iterator_category>
class TupleIter;

template<class Iter, szt I>
class TupleIter<Iter, I, std::forward_iterator_tag>
{
public:
    typedef std::forward_iterator_tag                               iterator_category;
    typedef decltype(get<I>(*Iter()))                               reference;
    typedef typename mt::removeRef<reference>::type                 value_type;
    typedef typename std::iterator_traits<Iter>::difference_type    difference_type;
    typedef value_type*                                             pointer;

    TupleIter() = default;
    TupleIter(const Iter& i)                                        : _i(i) {}

    TupleIter& operator++()                                         { ++_i; return *this; }
    TupleIter operator++(int)                                       { auto tmp = *this; ++*this; return tmp; }

    bool operator==(const TupleIter& rhs) const                     { return _i == rhs._i; }
    bool operator!=(const TupleIter& rhs) const                     { return !operator==(rhs); }
    
    reference operator*() const                                     { return get<I>(*_i); }
    pointer operator->() const                                      { return &get<I>(*_i); }
    operator Iter() const                                           { return _i; }

protected:
    Iter _i;
};

template<class Iter, szt I>
class TupleIter<Iter, I, std::bidirectional_iterator_tag> : public TupleIter<Iter, I, std::forward_iterator_tag>
{
    typedef TupleIter<Iter, I, std::forward_iterator_tag> Super;
    using Super::_i;
    
public:
    typedef std::bidirectional_iterator_tag                         iterator_category;

    TupleIter() = default;
    TupleIter(const Iter& i)                                        : Super(i) {}

    TupleIter& operator++()                                         { Super::operator++(); return *this; }
    TupleIter& operator--()                                         { --_i; return *this; }
    TupleIter operator++(int)                                       { auto tmp = *this; ++*this; return tmp; }
    TupleIter operator--(int)                                       { auto tmp = *this; --*this; return tmp; }
};

/// Ring iterator. See ringRange() to create.
template<class Range, class Iter>
class RingIter
{
public:
    typedef typename mt::iterOf<Range>::type                        RangeIter;
    typedef typename mt::iter_endOf<Range>::type                    RangeIterEnd;
    typedef std::bidirectional_iterator_tag                         iterator_category;
    typedef typename std::iterator_traits<Iter>::value_type         value_type;
    typedef typename std::iterator_traits<Iter>::difference_type    difference_type;
    typedef typename std::iterator_traits<Iter>::pointer            pointer;
    typedef typename std::iterator_traits<Iter>::reference          reference;

    RingIter(const RangeIter& begin, const RangeIterEnd& end, const Iter& cur, bool bEnd = false) :
        _begin(begin), _end(end), _curBegin(cur), _cur(cur), _bEnd(bEnd)
    {
        if (!_bEnd) _bEnd = _begin == _end;
        assert(_bEnd || _cur != _end);
    }

    RingIter& operator++()
    {
        assert(!_bEnd);
        ++_cur;
        if (_cur == _end) _cur = _begin;
        if (_cur == _curBegin) _bEnd = true;
        return *this;
    }

    RingIter& operator--()
    {
        assert(_begin != _end);
        assert(_bEnd || _cur != _curBegin);
        if (_bEnd) _bEnd = false;
        if (_cur == _begin) _cur = _end;
        --_cur;
        return *this;
    }

    RingIter operator++(int)                                        { auto tmp = *this; ++*this; return tmp; }
    RingIter operator--(int)                                        { auto tmp = *this; --*this; return tmp; }

    bool operator==(const RingIter& rhs) const                      { return _cur == rhs._cur && _bEnd == rhs._bEnd; }
    bool operator!=(const RingIter& rhs) const                      { return !operator==(rhs); }
    
    reference operator*() const                                     { return *_cur; }
    pointer operator->() const                                      { return _cur.operator->(); }
    operator const Iter&() const                                    { return _cur; }
    
    const Iter& iter() const                                        { return _cur; }
    
private:
    RangeIter _begin;
    RangeIterEnd _end;
    Iter _curBegin;
    Iter _cur;
    bool _bEnd;
};

/// Create an iterator adapter range that does one full cyclic loop starting at `cur` through the range
template<class Range, class Iter>
auto ringRange(Range&& range, const Iter& cur) ->
    Range_<RingIter<Range,Iter>, RingIter<Range,Iter>>
{
    typedef RingIter<Range,Iter> RingIter;
    return honey::range(RingIter(begin(range), end(range), cur), RingIter(begin(range), end(range), cur, true));
}

/// @}

}
