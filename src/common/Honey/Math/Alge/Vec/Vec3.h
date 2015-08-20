// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Swiz.h"

namespace honey
{

template<class Real, int Options>
struct matrix::priv::Traits<Vec<3,Real,Options>> : vec::priv::Traits<3,Real,Options,std::allocator<int8>>
{
    typedef vec::priv::StorageFields<Vec<3,Real,Options>> Storage;
};

namespace vec { namespace priv
{
    template<class Real, szt Align>
    struct StorageFieldsMixin<Real, 3, Align>
    {
        Real x;
        Real y;
        Real z;
    };
} }

namespace matrix { namespace priv
{
    template<class R, int O>
    void storageCopy(const R* a, Vec<3,R,O>& v)                     { v.x = a[0]; v.y = a[1]; v.z = a[2]; }
    template<class R, int O>
    void storageCopy(const Vec<3,R,O>& v, R* a)                     { a[0] = v.x; a[1] = v.y; a[2] = v.z; }

    template<class R, int O>
    void storageFill(Vec<3,R,O>& v, R f)                            { v.x = f; v.y = f; v.z = f; }
    template<class R, int O>
    void storageFillZero(Vec<3,R,O>& v)                             { v.x = 0; v.y = 0; v.z = 0; }

    template<class R, int O>
    bool storageEqual(const Vec<3,R,O>& lhs, const Vec<3,R,O>& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z; }
} }

/// 3D vector
template<class Real, int Options>
class Vec<3,Real,Options> : public VecBase<Vec<3,Real,Options>>
{
    typedef VecBase<Vec<3,Real,Options>> Super;
    using typename Super::Alge;
protected:
    typedef Vec<2,Real>                 Vec2;
    typedef Vec                         Vec3;
    typedef Vec<4,Real>                 Vec4;
    typedef VecSwizCon<2,Real,Options>  VecSwizCon2;
    typedef VecSwizCon<3,Real,Options>  VecSwizCon3;
    typedef VecSwizCon<4,Real,Options>  VecSwizCon4;
    typedef VecSwizRef<2,Real,Options>  VecSwizRef2;
    typedef VecSwizRef<3,Real,Options>  VecSwizRef3;
public:
    using Super::dot;
    using Super::x;
    using Super::y;
    using Super::z;
    
    /// No init
    Vec()                                                           {}
    Vec(Real x, Real y, Real z)                                     { this->x = x; this->y = y; this->z = z; }
    /// Construct uniform vector
    explicit Vec(Real scalar)                                       { this->fromScalar(scalar); }
    /// Construct from 2D vector
    explicit Vec(const Vec2& v, Real z = 0)                         { x = v.x; y = v.y; this->z = z; }
    /// Construct from vector ignoring the extra components
    explicit Vec(const Vec4& v)                                     { x = v.x; y = v.y; z = v.z; }
    /// Construct from vector of same dimension
    template<class T>
    Vec(const MatrixBase<T>& rhs)                                   { operator=(rhs); }

    template<class T>
    Vec& operator=(const MatrixBase<T>& rhs)                        { Super::operator=(rhs); return *this; }
    
    /// \name Specialized for optimization
    /// @{
    Real lengthSqr() const                                          { return x*x + y*y + z*z; }
    Real dot(const Vec& v) const                                    { return x*v.x + y*v.y + z*v.z; }
    /// @}

    /// Vector cross product
    Vec3 cross(const Vec3& v) const                                 { return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
    /// Vector cross product, result is normalized unit vector
    Vec3 crossUnit(const Vec3& v) const                             { return cross(v).normalize(); }

    /// Gram-Schmidt orthonormalization. Useful for re-normalizing an orthonormal basis to eliminate rounding errors.
    static void orthonormalize(Vec3& u, Vec3& v, Vec3& w)
    {
        u = u.normalize();

        Real dot0 = u.dot(v); 
        v -= dot0*u;
        v = v.normalize();

        Real dot1 = v.dot(w);
        dot0 = u.dot(w);
        w -= dot0*u + dot1*v;
        w = w.normalize();
    }

    /// Generate an orthonormal basis {u,v,this} (all unit length and perpendicular). Returns (Vec3 u, Vec3 v).  This vector must be unit length.
    tuple<Vec3,Vec3> orthonormalBasis() const
    {
        Vec u,v;
        const Vec3& w = *this;

        if (Alge::abs(w[0]) >= Alge::abs(w[1]))
        {
            // w.x or w.z is the largest magnitude component, swap them
            Real invLength = Alge::sqrtInv(w[0]*w[0] + w[2]*w[2]);
            u[0] = -w[2]*invLength;
            u[1] = 0;
            u[2] = +w[0]*invLength;
            v[0] = w[1]*u[2];
            v[1] = w[2]*u[0] - w[0]*u[2];
            v[2] = -w[1]*u[0];
        }
        else
        {
            // w.y or w.z is the largest magnitude component, swap them
            Real invLength = Alge::sqrtInv(w[1]*w[1] + w[2]*w[2]);
            u[0] = 0;
            u[1] = +w[2]*invLength;
            u[2] = -w[1]*invLength;
            v[0] = w[1]*u[2] - w[2]*u[1];
            v[1] = -w[0]*u[2];
            v[2] = w[0]*u[1];
        }
        return make_tuple(u,v);
    }

public:
    static const Vec zero;
    static const Vec one;
    static const Vec axisX;
    static const Vec axisY;
    static const Vec axisZ;
    static const Vec axis[3];

public:
    /// \name Const swizzle methods
    /// @{
    VecSwizCon2 xx() const                                          { return VecSwizCon2(x,x); }
    VecSwizCon3 xxx() const                                         { return VecSwizCon3(x,x,x); }
    VecSwizCon4 xxxx() const                                        { return VecSwizCon4(x,x,x,x); }
    VecSwizCon4 xxxy() const                                        { return VecSwizCon4(x,x,x,y); }
    VecSwizCon4 xxxz() const                                        { return VecSwizCon4(x,x,x,z); }
    VecSwizCon3 xxy() const                                         { return VecSwizCon3(x,x,y); }
    VecSwizCon4 xxyx() const                                        { return VecSwizCon4(x,x,y,x); }
    VecSwizCon4 xxyy() const                                        { return VecSwizCon4(x,x,y,y); }
    VecSwizCon4 xxyz() const                                        { return VecSwizCon4(x,x,y,z); }
    VecSwizCon3 xxz() const                                         { return VecSwizCon3(x,x,z); }
    VecSwizCon4 xxzx() const                                        { return VecSwizCon4(x,x,z,x); }
    VecSwizCon4 xxzy() const                                        { return VecSwizCon4(x,x,z,y); }
    VecSwizCon4 xxzz() const                                        { return VecSwizCon4(x,x,z,z); }
    VecSwizCon2 xy() const                                          { return VecSwizCon2(x,y); }
    VecSwizCon3 xyx() const                                         { return VecSwizCon3(x,y,x); }
    VecSwizCon4 xyxx() const                                        { return VecSwizCon4(x,y,x,x); }
    VecSwizCon4 xyxy() const                                        { return VecSwizCon4(x,y,x,y); }
    VecSwizCon4 xyxz() const                                        { return VecSwizCon4(x,y,x,z); }
    VecSwizCon3 xyy() const                                         { return VecSwizCon3(x,y,y); }
    VecSwizCon4 xyyx() const                                        { return VecSwizCon4(x,y,y,x); }
    VecSwizCon4 xyyy() const                                        { return VecSwizCon4(x,y,y,y); }
    VecSwizCon4 xyyz() const                                        { return VecSwizCon4(x,y,y,z); }
    VecSwizCon3 xyz() const                                         { return VecSwizCon3(x,y,z); }
    VecSwizCon4 xyzx() const                                        { return VecSwizCon4(x,y,z,x); }
    VecSwizCon4 xyzy() const                                        { return VecSwizCon4(x,y,z,y); }
    VecSwizCon4 xyzz() const                                        { return VecSwizCon4(x,y,z,z); }
    VecSwizCon2 xz() const                                          { return VecSwizCon2(x,z); }
    VecSwizCon3 xzx() const                                         { return VecSwizCon3(x,z,x); }
    VecSwizCon4 xzxx() const                                        { return VecSwizCon4(x,z,x,x); }
    VecSwizCon4 xzxy() const                                        { return VecSwizCon4(x,z,x,y); }
    VecSwizCon4 xzxz() const                                        { return VecSwizCon4(x,z,x,z); }
    VecSwizCon3 xzy() const                                         { return VecSwizCon3(x,z,y); }
    VecSwizCon4 xzyx() const                                        { return VecSwizCon4(x,z,y,x); }
    VecSwizCon4 xzyy() const                                        { return VecSwizCon4(x,z,y,y); }
    VecSwizCon4 xzyz() const                                        { return VecSwizCon4(x,z,y,z); }
    VecSwizCon3 xzz() const                                         { return VecSwizCon3(x,z,z); }
    VecSwizCon4 xzzx() const                                        { return VecSwizCon4(x,z,z,x); }
    VecSwizCon4 xzzy() const                                        { return VecSwizCon4(x,z,z,y); }
    VecSwizCon4 xzzz() const                                        { return VecSwizCon4(x,z,z,z); }
    VecSwizCon2 yx() const                                          { return VecSwizCon2(y,x); }
    VecSwizCon3 yxx() const                                         { return VecSwizCon3(y,x,x); }
    VecSwizCon4 yxxx() const                                        { return VecSwizCon4(y,x,x,x); }
    VecSwizCon4 yxxy() const                                        { return VecSwizCon4(y,x,x,y); }
    VecSwizCon4 yxxz() const                                        { return VecSwizCon4(y,x,x,z); }
    VecSwizCon3 yxy() const                                         { return VecSwizCon3(y,x,y); }
    VecSwizCon4 yxyx() const                                        { return VecSwizCon4(y,x,y,x); }
    VecSwizCon4 yxyy() const                                        { return VecSwizCon4(y,x,y,y); }
    VecSwizCon4 yxyz() const                                        { return VecSwizCon4(y,x,y,z); }
    VecSwizCon3 yxz() const                                         { return VecSwizCon3(y,x,z); }
    VecSwizCon4 yxzx() const                                        { return VecSwizCon4(y,x,z,x); }
    VecSwizCon4 yxzy() const                                        { return VecSwizCon4(y,x,z,y); }
    VecSwizCon4 yxzz() const                                        { return VecSwizCon4(y,x,z,z); }
    VecSwizCon2 yy() const                                          { return VecSwizCon2(y,y); }
    VecSwizCon3 yyx() const                                         { return VecSwizCon3(y,y,x); }
    VecSwizCon4 yyxx() const                                        { return VecSwizCon4(y,y,x,x); }
    VecSwizCon4 yyxy() const                                        { return VecSwizCon4(y,y,x,y); }
    VecSwizCon4 yyxz() const                                        { return VecSwizCon4(y,y,x,z); }
    VecSwizCon3 yyy() const                                         { return VecSwizCon3(y,y,y); }
    VecSwizCon4 yyyx() const                                        { return VecSwizCon4(y,y,y,x); }
    VecSwizCon4 yyyy() const                                        { return VecSwizCon4(y,y,y,y); }
    VecSwizCon4 yyyz() const                                        { return VecSwizCon4(y,y,y,z); }
    VecSwizCon3 yyz() const                                         { return VecSwizCon3(y,y,z); }
    VecSwizCon4 yyzx() const                                        { return VecSwizCon4(y,y,z,x); }
    VecSwizCon4 yyzy() const                                        { return VecSwizCon4(y,y,z,y); }
    VecSwizCon4 yyzz() const                                        { return VecSwizCon4(y,y,z,z); }
    VecSwizCon2 yz() const                                          { return VecSwizCon2(y,z); }
    VecSwizCon3 yzx() const                                         { return VecSwizCon3(y,z,x); }
    VecSwizCon4 yzxx() const                                        { return VecSwizCon4(y,z,x,x); }
    VecSwizCon4 yzxy() const                                        { return VecSwizCon4(y,z,x,y); }
    VecSwizCon4 yzxz() const                                        { return VecSwizCon4(y,z,x,z); }
    VecSwizCon3 yzy() const                                         { return VecSwizCon3(y,z,y); }
    VecSwizCon4 yzyx() const                                        { return VecSwizCon4(y,z,y,x); }
    VecSwizCon4 yzyy() const                                        { return VecSwizCon4(y,z,y,y); }
    VecSwizCon4 yzyz() const                                        { return VecSwizCon4(y,z,y,z); }
    VecSwizCon3 yzz() const                                         { return VecSwizCon3(y,z,z); }
    VecSwizCon4 yzzx() const                                        { return VecSwizCon4(y,z,z,x); }
    VecSwizCon4 yzzy() const                                        { return VecSwizCon4(y,z,z,y); }
    VecSwizCon4 yzzz() const                                        { return VecSwizCon4(y,z,z,z); }
    VecSwizCon2 zx() const                                          { return VecSwizCon2(z,x); }
    VecSwizCon3 zxx() const                                         { return VecSwizCon3(z,x,x); }
    VecSwizCon4 zxxx() const                                        { return VecSwizCon4(z,x,x,x); }
    VecSwizCon4 zxxy() const                                        { return VecSwizCon4(z,x,x,y); }
    VecSwizCon4 zxxz() const                                        { return VecSwizCon4(z,x,x,z); }
    VecSwizCon3 zxy() const                                         { return VecSwizCon3(z,x,y); }
    VecSwizCon4 zxyx() const                                        { return VecSwizCon4(z,x,y,x); }
    VecSwizCon4 zxyy() const                                        { return VecSwizCon4(z,x,y,y); }
    VecSwizCon4 zxyz() const                                        { return VecSwizCon4(z,x,y,z); }
    VecSwizCon3 zxz() const                                         { return VecSwizCon3(z,x,z); }
    VecSwizCon4 zxzx() const                                        { return VecSwizCon4(z,x,z,x); }
    VecSwizCon4 zxzy() const                                        { return VecSwizCon4(z,x,z,y); }
    VecSwizCon4 zxzz() const                                        { return VecSwizCon4(z,x,z,z); }
    VecSwizCon2 zy() const                                          { return VecSwizCon2(z,y); }
    VecSwizCon3 zyx() const                                         { return VecSwizCon3(z,y,x); }
    VecSwizCon4 zyxx() const                                        { return VecSwizCon4(z,y,x,x); }
    VecSwizCon4 zyxy() const                                        { return VecSwizCon4(z,y,x,y); }
    VecSwizCon4 zyxz() const                                        { return VecSwizCon4(z,y,x,z); }
    VecSwizCon3 zyy() const                                         { return VecSwizCon3(z,y,y); }
    VecSwizCon4 zyyx() const                                        { return VecSwizCon4(z,y,y,x); }
    VecSwizCon4 zyyy() const                                        { return VecSwizCon4(z,y,y,y); }
    VecSwizCon4 zyyz() const                                        { return VecSwizCon4(z,y,y,z); }
    VecSwizCon3 zyz() const                                         { return VecSwizCon3(z,y,z); }
    VecSwizCon4 zyzx() const                                        { return VecSwizCon4(z,y,z,x); }
    VecSwizCon4 zyzy() const                                        { return VecSwizCon4(z,y,z,y); }
    VecSwizCon4 zyzz() const                                        { return VecSwizCon4(z,y,z,z); }
    VecSwizCon2 zz() const                                          { return VecSwizCon2(z,z); }
    VecSwizCon3 zzx() const                                         { return VecSwizCon3(z,z,x); }
    VecSwizCon4 zzxx() const                                        { return VecSwizCon4(z,z,x,x); }
    VecSwizCon4 zzxy() const                                        { return VecSwizCon4(z,z,x,y); }
    VecSwizCon4 zzxz() const                                        { return VecSwizCon4(z,z,x,z); }
    VecSwizCon3 zzy() const                                         { return VecSwizCon3(z,z,y); }
    VecSwizCon4 zzyx() const                                        { return VecSwizCon4(z,z,y,x); }
    VecSwizCon4 zzyy() const                                        { return VecSwizCon4(z,z,y,y); }
    VecSwizCon4 zzyz() const                                        { return VecSwizCon4(z,z,y,z); }
    VecSwizCon3 zzz() const                                         { return VecSwizCon3(z,z,z); }
    VecSwizCon4 zzzx() const                                        { return VecSwizCon4(z,z,z,x); }
    VecSwizCon4 zzzy() const                                        { return VecSwizCon4(z,z,z,y); }
    VecSwizCon4 zzzz() const                                        { return VecSwizCon4(z,z,z,z); }


    /// \name Mutable swizzle methods
    /// @{
    VecSwizRef2 xy()                                                { return VecSwizRef2(x,y); }
    VecSwizRef3 xyz()                                               { return VecSwizRef3(x,y,z); }
    VecSwizRef2 xz()                                                { return VecSwizRef2(x,z); }
    VecSwizRef3 xzy()                                               { return VecSwizRef3(x,z,y); }
    VecSwizRef2 yx()                                                { return VecSwizRef2(y,x); }
    VecSwizRef3 yxz()                                               { return VecSwizRef3(y,x,z); }
    VecSwizRef2 yz()                                                { return VecSwizRef2(y,z); }
    VecSwizRef3 yzx()                                               { return VecSwizRef3(y,z,x); }
    VecSwizRef2 zx()                                                { return VecSwizRef2(z,x); }
    VecSwizRef3 zxy()                                               { return VecSwizRef3(z,x,y); }
    VecSwizRef2 zy()                                                { return VecSwizRef2(z,y); }
    VecSwizRef3 zyx()                                               { return VecSwizRef3(z,y,x); }
    /// @}
};

template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::zero          (0);
template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::one           (1);
template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::axisX         (1, 0, 0);
template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::axisY         (0, 1, 0);
template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::axisZ         (0, 0, 1);
template<class R, int O> const Vec<3,R,O> Vec<3,R,O>::axis[3]       = { axisX, axisY, axisZ };

/** \cond */
/// \name Specialized for optimization
/// @{
template<class R, int Opt>
struct priv::map_impl<Vec<3,R,Opt>, Vec<3,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& v, O&& o, Func&& f)                         { o.x = f(v.x); o.y = f(v.y); o.z = f(v.z); return forward<O>(o); }
};

template<class R, int Opt>
struct priv::map_impl<Vec<3,R,Opt>, Vec<3,R,Opt>, Vec<3,R,Opt>>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& v, T2&& rhs, O&& o, Func&& f)               { o.x = f(v.x,rhs.x); o.y = f(v.y,rhs.y); o.z = f(v.z,rhs.z); return forward<O>(o); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<3,R,O>, Accum_>
{
    template<class T, class Accum, class Func>
    static Accum_ func(T&& v, Accum&& initVal, Func&& f)            { return f(f(f(forward<Accum>(initVal), v.x), v.y), v.z); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<3,R,O>, Accum_, Vec<3,R,O>>
{
    template<class T, class T2, class Accum, class Func>
    static Accum_ func(T&& v, T2&& rhs, Accum&& initVal, Func&& f)  { return f(f(f(forward<Accum>(initVal), v.x, rhs.x), v.y, rhs.y), v.z, rhs.z); }
};
/// @}
/** \endcond */

/// 3D const swizzle vector
template<class Real, int Options>
class VecSwizCon<3,Real,Options> : public VecSwizConBase<VecSwizCon<3,Real,Options>>
{
public:
    VecSwizCon(Real x, Real y, Real z)                              { this->x = x; this->y = y; this->z = z; }
};

/// 3D mutable swizzle vector
template<class Real, int Options>
class VecSwizRef<3,Real,Options> : public VecSwizRefBase<VecSwizRef<3,Real,Options>>
{
    template<class> friend class VecSwizRefBase;
public:
    typedef VecSwizRefBase<VecSwizRef<3,Real,Options>> Super;
    using Super::operator=;
    using Super::x;
    using Super::y;
    using Super::z;
    
    VecSwizRef(Real& x, Real& y, Real& z)                           : rx(x), ry(y), rz(z) { this->x = x; this->y = y; this->z = z; }

    VecSwizRef& commit()                                            { rx = x; ry = y; rz = z; return *this; }

private:
    Real& rx;
    Real& ry;
    Real& rz;
};


/// 3D column vector types
typedef Vec<3>                                  Vec3;
typedef Vec<3,Float>                            Vec3_f;
typedef Vec<3,Double>                           Vec3_d;

/// 3D row vector types
typedef Vec<3,Real,matrix::Option::vecRow>      VecRow3;
typedef Vec<3,Float,matrix::Option::vecRow>     VecRow3_f;
typedef Vec<3,Double,matrix::Option::vecRow>    VecRow3_d;

}

#include "Honey/Math/Alge/Vec/platform/Vec3.h"