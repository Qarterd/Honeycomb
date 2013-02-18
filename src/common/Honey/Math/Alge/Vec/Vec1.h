// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Vec.h"

namespace honey
{

template<class Real, int Options>
struct matrix::priv::Traits<Vec<1,Real,Options>> : vec::priv::Traits<1,Real,Options,std::allocator<int8>>
{
    typedef vec::priv::StorageFields<Vec<1,Real,Options>> Storage;
};

namespace vec { namespace priv
{
    template<class Real, int Align>
    struct StorageFieldsMixin<Real, 1, Align>
    {
        operator Real() const                                       { return x; }
        Real x;
    };
} }

namespace matrix { namespace priv
{
    template<class R, int O>
    void storageCopy(const R* a, Vec<1,R,O>& v)                     { v.x = a[0]; }
    template<class R, int O>
    void storageCopy(const Vec<1,R,O>& v, R* a)                     { a[0] = v.x; }

    template<class R, int O>
    void storageFill(Vec<1,R,O>& v, R f)                            { v.x = f; }
    template<class R, int O>
    void storageFillZero(Vec<1,R,O>& v)                             { v.x = 0; }

    template<class R, int O>
    bool storageEqual(const Vec<1,R,O>& lhs, const Vec<1,R,O>& rhs) { return lhs.x == rhs.x; }
} }

/// 1D vector
template<class Real, int Options>
class Vec<1,Real,Options> : public VecBase<Vec<1,Real,Options>>
{
    typedef VecBase<Vec<1,Real,Options>> Super;
public:
    using Super::dot;
    using Super::operator*;
    using Super::operator*=;
    using Super::operator/;
    using Super::operator/=;
    using Super::x;
    
    /// No init
    Vec()                                                           {}
    Vec(Real x)                                                     { this->x = x; }
    /// Construct from vector of same dimension
    template<class T>
    Vec(const MatrixBase<T>& rhs)                                   { operator=(rhs); }
    
    template<class T>
    Vec& operator=(const MatrixBase<T>& rhs)                        { Super::operator=(rhs); return *this; }

    /// Implicit conversion to real causes ambiguity with int
    Vec  operator* (int rhs) const                                  { return operator*(Real(rhs)); }
    Vec& operator*=(int rhs)                                        { return operator*=(Real(rhs)); }
    Vec  operator/ (int rhs) const                                  { return operator/(Real(rhs)); }
    Vec& operator/=(int rhs)                                        { return operator/=(Real(rhs)); }
    friend Vec operator*(int lhs, const Vec& rhs)                   { return operator*(Real(lhs), rhs); }

    /// \name Specialized for optimization
    /// @{
    Real lengthSqr() const                                          { return x*x; }
    Real length() const                                             { return x; }
    Real dot(const Vec& v) const                                    { return x*v.x; }
    /// @}

public:
    static const Vec zero;
    static const Vec one;
    static const Vec axisX;
    static const Vec axis[1];
};

template<class R, int O> const Vec<1,R,O> Vec<1,R,O>::zero          (0);
template<class R, int O> const Vec<1,R,O> Vec<1,R,O>::one           (1);
template<class R, int O> const Vec<1,R,O> Vec<1,R,O>::axisX         (1);
template<class R, int O> const Vec<1,R,O> Vec<1,R,O>::axis[1]       = { axisX };

/** \cond */
/// \name Specialized for optimization
/// @{
template<class R, int Opt>
struct priv::map_impl<Vec<1,R,Opt>, Vec<1,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& v, O&& o, Func&& f)                         { o.x = f(v.x); return forward<O>(o); }
};

template<class R, int Opt>
struct priv::map_impl<Vec<1,R,Opt>, Vec<1,R,Opt>, Vec<1,R,Opt>>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& v, T2&& rhs, O&& o, Func&& f)               { o.x = f(v.x,rhs.x); return forward<O>(o); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<1,R,O>, Accum_>
{
    template<class T, class Accum, class Func>
    static Accum_ func(T&& v, Accum&& initVal, Func&& f)            { return f(forward<Accum>(initVal), v.x); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<1,R,O>, Accum_, Vec<1,R,O>>
{
    template<class T, class T2, class Accum, class Func>
    static Accum_ func(T&& v, T2&& rhs, Accum&& initVal, Func&& f)  { return f(forward<Accum>(initVal), v.x, rhs.x); }
};
/// @}
/** \endcond */

/// 1D vector types
typedef Vec<1>          Vec1;
typedef Vec<1,Float>    Vec1_f;
typedef Vec<1,Double>   Vec1_d;


/// Matrix 1x1 vector
template<class Real, int Options>
class Matrix<1,1,Real,Options> : public Vec<1,Real,Options>
{
    typedef Vec<1,Real,Options> Super;
    MATRIX_VEC_ADAPTER
};

}
