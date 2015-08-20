// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Swiz.h"

namespace honey
{

template<class Real, int Options>
struct matrix::priv::Traits<Vec<2,Real,Options>> : vec::priv::Traits<2,Real,Options,std::allocator<int8>>
{
    typedef vec::priv::StorageFields<Vec<2,Real,Options>> Storage;
};

namespace vec { namespace priv
{
    template<class Real, szt Align>
    struct StorageFieldsMixin<Real, 2, Align>
    {
        Real x;
        Real y;
    };
} }

namespace matrix { namespace priv
{
    template<class R, int O>
    void storageCopy(const R* a, Vec<2,R,O>& v)                     { v.x = a[0]; v.y = a[1]; }
    template<class R, int O>
    void storageCopy(const Vec<2,R,O>& v, R* a)                     { a[0] = v.x; a[1] = v.y; }

    template<class R, int O>
    void storageFill(Vec<2,R,O>& v, R f)                            { v.x = f; v.y = f; }
    template<class R, int O>
    void storageFillZero(Vec<2,R,O>& v)                             { v.x = 0; v.y = 0; }

    template<class R, int O>
    bool storageEqual(const Vec<2,R,O>& lhs, const Vec<2,R,O>& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
} }

/// 2D vector
template<class Real, int Options>
class Vec<2,Real,Options> : public VecBase<Vec<2,Real,Options>>
{
    typedef VecBase<Vec<2,Real,Options>> Super;
protected:
    typedef Vec                         Vec2;
    typedef Vec<3,Real>                 Vec3;
    typedef Vec<4,Real>                 Vec4;
    typedef VecSwizCon<2,Real,Options>  VecSwizCon2;
    typedef VecSwizCon<3,Real,Options>  VecSwizCon3;
    typedef VecSwizCon<4,Real,Options>  VecSwizCon4;
    typedef VecSwizRef<2,Real,Options>  VecSwizRef2;
public:
    using Super::dot;
    using Super::x;
    using Super::y;
    
    /// No init
    Vec()                                                           {}
    Vec(Real x, Real y)                                             { this->x = x; this->y = y; }
    /// Construct uniform vector
    explicit Vec(Real scalar)                                       { this->fromScalar(scalar); }
    /// Construct from vector ignoring the extra components
    explicit Vec(const Vec3& v)                                     { x = v.x; y = v.y; }
    explicit Vec(const Vec4& v)                                     { x = v.x; y = v.y; }
    /// Construct from vector of same dimension
    template<class T>
    Vec(const MatrixBase<T>& rhs)                                   { operator=(rhs); }
    
    template<class T>
    Vec& operator=(const MatrixBase<T>& rhs)                        { Super::operator=(rhs); return *this; }

    /// \name Specialized for optimization
    /// @{
    Real lengthSqr() const                                          { return x*x + y*y; }
    Real dot(const Vec& v) const                                    { return x*v.x + y*v.y; }
    /// @}

    /// Vector cross product
    Real cross(const Vec& v) const                                  { return x*v.y - y*v.x; }

    /// Get the left-perpendicular vector
    Vec normal() const                                              { return Vec(-y, x); }

public:
    static const Vec zero;
    static const Vec one;
    static const Vec axisX;
    static const Vec axisY;
    static const Vec axis[2];

public:
    /// \name Const swizzle methods
    /// @{
    VecSwizCon2 xx() const                                          { return VecSwizCon2(x,x); }
    VecSwizCon3 xxx() const                                         { return VecSwizCon3(x,x,x); }
    VecSwizCon4 xxxx() const                                        { return VecSwizCon4(x,x,x,x); }
    VecSwizCon4 xxxy() const                                        { return VecSwizCon4(x,x,x,y); }
    VecSwizCon3 xxy() const                                         { return VecSwizCon3(x,x,y); }
    VecSwizCon4 xxyx() const                                        { return VecSwizCon4(x,x,y,x); }
    VecSwizCon4 xxyy() const                                        { return VecSwizCon4(x,x,y,y); }
    VecSwizCon2 xy() const                                          { return VecSwizCon2(x,y); }
    VecSwizCon3 xyx() const                                         { return VecSwizCon3(x,y,x); }
    VecSwizCon4 xyxx() const                                        { return VecSwizCon4(x,y,x,x); }
    VecSwizCon4 xyxy() const                                        { return VecSwizCon4(x,y,x,y); }
    VecSwizCon3 xyy() const                                         { return VecSwizCon3(x,y,y); }
    VecSwizCon4 xyyx() const                                        { return VecSwizCon4(x,y,y,x); }
    VecSwizCon4 xyyy() const                                        { return VecSwizCon4(x,y,y,y); }
    VecSwizCon2 yx() const                                          { return VecSwizCon2(y,x); }
    VecSwizCon3 yxx() const                                         { return VecSwizCon3(y,x,x); }
    VecSwizCon4 yxxx() const                                        { return VecSwizCon4(y,x,x,x); }
    VecSwizCon4 yxxy() const                                        { return VecSwizCon4(y,x,x,y); }
    VecSwizCon3 yxy() const                                         { return VecSwizCon3(y,x,y); }
    VecSwizCon4 yxyx() const                                        { return VecSwizCon4(y,x,y,x); }
    VecSwizCon4 yxyy() const                                        { return VecSwizCon4(y,x,y,y); }
    VecSwizCon2 yy() const                                          { return VecSwizCon2(y,y); }
    VecSwizCon3 yyx() const                                         { return VecSwizCon3(y,y,x); }
    VecSwizCon4 yyxx() const                                        { return VecSwizCon4(y,y,x,x); }
    VecSwizCon4 yyxy() const                                        { return VecSwizCon4(y,y,x,y); }
    VecSwizCon3 yyy() const                                         { return VecSwizCon3(y,y,y); }
    VecSwizCon4 yyyx() const                                        { return VecSwizCon4(y,y,y,x); }
    VecSwizCon4 yyyy() const                                        { return VecSwizCon4(y,y,y,y); }
    /// @}

    /// \name Mutable swizzle methods
    /// @{
    VecSwizRef2 xy()                                                { return VecSwizRef2(x,y); }
    VecSwizRef2 yx()                                                { return VecSwizRef2(y,x); }
    /// @}
};

template<class R, int O> const Vec<2,R,O> Vec<2,R,O>::zero          (0);
template<class R, int O> const Vec<2,R,O> Vec<2,R,O>::one           (1);
template<class R, int O> const Vec<2,R,O> Vec<2,R,O>::axisX         (1, 0);
template<class R, int O> const Vec<2,R,O> Vec<2,R,O>::axisY         (0, 1);
template<class R, int O> const Vec<2,R,O> Vec<2,R,O>::axis[2]       = { axisX, axisY };

/** \cond */
/// \name Specialized for optimization
/// @{
template<class R, int Opt>
struct priv::map_impl<Vec<2,R,Opt>, Vec<2,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& v, O&& o, Func&& f)                         { o.x = f(v.x); o.y = f(v.y); return forward<O>(o); }
};

template<class R, int Opt>
struct priv::map_impl<Vec<2,R,Opt>, Vec<2,R,Opt>, Vec<2,R,Opt>>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& v, T2&& rhs, O&& o, Func&& f)               { o.x = f(v.x,rhs.x); o.y = f(v.y,rhs.y); return forward<O>(o); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<2,R,O>, Accum_>
{
    template<class T, class Accum, class Func>
    static Accum_ func(T&& v, Accum&& initVal, Func&& f)            { return f(f(forward<Accum>(initVal), v.x), v.y); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<2,R,O>, Accum_, Vec<2,R,O>>
{
    template<class T, class T2, class Accum, class Func>
    static Accum_ func(T&& v, T2&& rhs, Accum&& initVal, Func&& f)  { return f(f(forward<Accum>(initVal), v.x, rhs.x), v.y, rhs.y); }
};
/// @}
/** \endcond */

/// 2D const swizzle vector
template<class Real, int Options>
class VecSwizCon<2,Real,Options> : public VecSwizConBase<VecSwizCon<2,Real,Options>>
{
public:
    VecSwizCon(Real x, Real y)                                      { this->x = x; this->y = y; }
};

/// 2D mutable swizzle vector
template<class Real, int Options>
class VecSwizRef<2,Real,Options> : public VecSwizRefBase<VecSwizRef<2,Real,Options>>
{
    template<class> friend class VecSwizRefBase;
public:
    typedef VecSwizRefBase<VecSwizRef<2,Real,Options>> Super;
    using Super::operator=;
    using Super::x;
    using Super::y;
    
    VecSwizRef(Real& x, Real& y)                                    : rx(x), ry(y) { this->x = x; this->y = y; }

    VecSwizRef& commit()                                            { rx = x; ry = y; return *this; }

private:
    Real& rx;
    Real& ry;
};

/// 2D column vector types
typedef Vec<2>                                  Vec2;
typedef Vec<2,Float>                            Vec2_f;
typedef Vec<2,Double>                           Vec2_d;

/// 2D row vector types
typedef Vec<2,Real,matrix::Option::vecRow>      VecRow2;
typedef Vec<2,Float,matrix::Option::vecRow>     VecRow2_f;
typedef Vec<2,Double,matrix::Option::vecRow>    VecRow2_d;

}

#include "Honey/Math/Alge/Vec/platform/Vec2.h"