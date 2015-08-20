// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Vec.h"

namespace honey
{

/// Vector for const swizzle operators
template<sdt Dim, class Real, int Options> class VecSwizCon;

template<sdt Dim, class Real, int Options>
struct matrix::priv::Traits<VecSwizCon<Dim,Real,Options>> : Traits<Vec<Dim,Real,Options>> {};

template<class SwizT>
class VecSwizConBase : public Vec<  matrix::priv::Traits<SwizT>::dim, typename matrix::priv::Traits<SwizT>::Real,
                                    matrix::priv::Traits<SwizT>::options>
{
private:
    SwizT& fromZero();
    SwizT& fromScalar(Real f);
    SwizT& operator=(const VecSwizConBase& rhs);
    template<class T>
    SwizT& operator+=(const MatrixBase<T>& rhs);
    template<class T>
    SwizT& operator-=(const MatrixBase<T>& rhs);
    SwizT& operator*=(Real rhs);
    SwizT& operator/=(Real rhs);
    SwizT& elemAddEq(Real rhs);
    SwizT& elemSubEq(Real rhs);
    template<class T>
    SwizT& elemMulEq(const MatrixBase<T>& rhs);
    template<class T>
    SwizT& elemDivEq(const MatrixBase<T>& rhs);
};

/** \cond */
template<class T_, sdt D, class R, int Opt>
struct priv::map_impl<T_, VecSwizCon<D,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&&, O&&, Func&&)                               { static_assert(!mt::True<T>::value, "Can't map with const swizzle output"); }
};

template<class T_, sdt D, class R, int Opt, class T2_>
struct priv::map_impl<T_, VecSwizCon<D,R,Opt>, T2_>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&&, T2&&, O&&, Func&&)                         { static_assert(!mt::True<T>::value, "Can't map with const swizzle output"); }
};
/** \endcond */

/// Vector reference holder for mutable swizzle operators
template<sdt Dim, class Real, int Options> class VecSwizRef;

template<sdt Dim, class Real, int Options>
struct matrix::priv::Traits<VecSwizRef<Dim,Real,Options>> : Traits<Vec<Dim,Real,Options>> {};

template<class SwizT>
class VecSwizRefBase : public Vec<  matrix::priv::Traits<SwizT>::dim, typename matrix::priv::Traits<SwizT>::Real,
                                    matrix::priv::Traits<SwizT>::options>
{
public:
    typedef Vec<matrix::priv::Traits<SwizT>::dim, typename matrix::priv::Traits<SwizT>::Real,
                matrix::priv::Traits<SwizT>::options> Super;

    /// Allow scalar ops
    SwizT& operator=(Real rhs)                                      { return fromScalar(rhs); }
    SwizT& operator+=(Real rhs)                                     { return elemAddEq(rhs); }
    SwizT& operator-=(Real rhs)                                     { return elemSubEq(rhs); }

    /// Wrapper ops
    SwizT& fromZero()                                               { Super::fromZero(); return swiz().commit(); }
    SwizT& fromScalar(Real f)                                       { Super::fromScalar(f); return swiz().commit(); }
    SwizT& operator=(const VecSwizRefBase& rhs)                     { Super::operator=(rhs); return swiz().commit(); }
    template<class T>
    SwizT& operator=(const MatrixBase<T>& rhs)                      { Super::operator=(rhs.subc()); return swiz().commit(); }
    template<class T>
    SwizT& operator+=(const MatrixBase<T>& rhs)                     { Super::operator+=(rhs.subc()); return swiz().commit(); }
    template<class T>
    SwizT& operator-=(const MatrixBase<T>& rhs)                     { Super::operator-=(rhs.subc()); return swiz().commit(); }
    SwizT& operator*=(Real rhs)                                     { Super::operator*=(rhs); return swiz().commit(); }
    SwizT& operator/=(Real rhs)                                     { Super::operator/=(rhs); return swiz().commit(); }
    SwizT& elemAddEq(Real rhs)                                      { Super::elemAddEq(rhs); return swiz().commit(); }
    SwizT& elemSubEq(Real rhs)                                      { Super::elemSubEq(rhs); return swiz().commit(); }
    template<class T>
    SwizT& elemMulEq(const MatrixBase<T>& rhs)                      { Super::elemMulEq(rhs.subc()); return swiz().commit(); }
    template<class T>
    SwizT& elemDivEq(const MatrixBase<T>& rhs)                      { Super::elemDivEq(rhs.subc()); return swiz().commit(); }

    /// Get subclass
    const SwizT& swiz() const                                       { return static_cast<const SwizT&>(*this); }
    SwizT& swiz()                                                   { return static_cast<SwizT&>(*this); }
};

/** \cond */
template<class T_, sdt D, class R, int Opt>
struct priv::map_impl<T_, VecSwizRef<D,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& v, O&& o, Func&& f)                         { map(forward<T>(v), forward<typename O::Super>(o), forward<Func>(f)); o.swiz().commit(); return forward<O>(o); }
};

template<class T_, sdt D, class R, int Opt, class T2_>
struct priv::map_impl<T_, VecSwizRef<D,R,Opt>, T2_>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& v, T2&& v2, O&& o, Func&& f)                { map(forward<T>(v), forward<T2>(v2), forward<typename O::Super>(o), forward<Func>(f)); o.swiz().commit(); return forward<O>(o); }
};
/** \endcond */

}
