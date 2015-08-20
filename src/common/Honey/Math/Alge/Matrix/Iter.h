// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

namespace honey { namespace matrix
{

/// Matrix element iterator
template<class Matrix>
class Iter
{
public:
    /// Our element access is const if matrix is const
    typedef typename std::conditional<std::is_const<Matrix>::value, const typename Matrix::ElemT, typename Matrix::ElemT>::type
                                                                    ElemT;

    typedef std::random_access_iterator_tag                         iterator_category;
    typedef ElemT                                                   value_type;
    typedef sdt                                                     difference_type;
    typedef ElemT*                                                  pointer;
    typedef ElemT&                                                  reference;

    Iter()                                                          : _m(nullptr) {}
    Iter(Matrix& m, sdt i)                                          : _m(&m), _i(i) {}

    Iter& operator++()                                              { ++_i; return *this; }
    Iter& operator--()                                              { --_i; return *this; }
    Iter operator++(int)                                            { auto tmp = *this; ++*this; return tmp; }
    Iter operator--(int)                                            { auto tmp = *this; --*this; return tmp; }
    Iter& operator+=(difference_type rhs)                           { _i += rhs; return *this; }
    Iter& operator-=(difference_type rhs)                           { _i -= rhs; return *this; }
    Iter operator+(difference_type rhs) const                       { auto tmp = *this; tmp+=rhs; return tmp; }
    Iter operator-(difference_type rhs) const                       { auto tmp = *this; tmp-=rhs; return tmp; }
    difference_type operator-(const Iter& rhs) const                { return _i - rhs._i; }

    bool operator==(const Iter& rhs) const                          { return _i == rhs._i; }
    bool operator!=(const Iter& rhs) const                          { return _i != rhs._i; }
    bool operator< (const Iter& rhs) const                          { return _i <  rhs._i; }
    bool operator> (const Iter& rhs) const                          { return _i >  rhs._i; }
    bool operator<=(const Iter& rhs) const                          { return _i <= rhs._i; }
    bool operator>=(const Iter& rhs) const                          { return _i >= rhs._i; }

    reference operator*() const                                     { return (*_m)(_i); }
    operator difference_type() const                                { return _i; }

private:
    Matrix* _m;
    sdt _i;
};

} }