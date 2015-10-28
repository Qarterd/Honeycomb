// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Swiz.h"
#include "Honey/Math/Alge/Vec/Vec3.h"

namespace honey
{

template<class Real, int Options>
struct matrix::priv::Traits<Vec<4,Real,Options>> : vec::priv::Traits<4,Real,Options,std::allocator<int8>>
{
    typedef vec::priv::StorageFields<Vec<4,Real,Options>> Storage;
};

namespace vec { namespace priv
{
    template<class Real, szt Align>
    struct StorageFieldsMixin<Real, 4, Align>
    {
        Real x;
        Real y;
        Real z;
        Real w;
    };

    template<class Real>
    struct StorageFieldsMixin<Real, 4, 16>
    {
        ALIGN(16) Real x;
        Real y;
        Real z;
        Real w;
    };
} }

namespace matrix { namespace priv
{
    template<class R, int O>
    void storageCopy(const R* a, Vec<4,R,O>& v)                     { v.x = a[0]; v.y = a[1]; v.z = a[2]; v.w = a[3]; }
    template<class R, int O>
    void storageCopy(const Vec<4,R,O>& v, R* a)                     { a[0] = v.x; a[1] = v.y; a[2] = v.z; a[3] = v.w; }

    template<class R, int O>
    void storageFill(Vec<4,R,O>& v, R f)                            { v.x = f; v.y = f; v.z = f; v.w = f; }
    template<class R, int O>
    void storageFillZero(Vec<4,R,O>& v)                             { v.x = 0; v.y = 0; v.z = 0; v.w = 0; }

    template<class R, int O>
    bool storageEqual(const Vec<4,R,O>& lhs, const Vec<4,R,O>& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w; }
} }

/// 4D vector
template<class Real, int Options>
class Vec<4,Real,Options> : public VecBase<Vec<4,Real,Options>>
{
    typedef VecBase<Vec<4,Real,Options>> Super;
protected:
    typedef Vec<2,Real>                 Vec2;
    typedef Vec<3,Real>                 Vec3;
    typedef Vec                         Vec4;
    typedef VecSwizCon<2,Real,Options>  VecSwizCon2;
    typedef VecSwizCon<3,Real,Options>  VecSwizCon3;
    typedef VecSwizCon<4,Real,Options>  VecSwizCon4;
    typedef VecSwizRef<2,Real,Options>  VecSwizRef2;
    typedef VecSwizRef<3,Real,Options>  VecSwizRef3;
    typedef VecSwizRef<4,Real,Options>  VecSwizRef4;
public:
    using Super::dot;
    using Super::x;
    using Super::y;
    using Super::z;
    using Super::w;
    
    /// No init
    Vec()                                                           {}
    Vec(Real x, Real y, Real z, Real w)                             { this->x = x; this->y = y; this->z = z; this->w = w; }
    /// Construct uniform vector
    explicit Vec(Real scalar)                                       { this->fromScalar(scalar); }
    /// Construct from 2D vector
    explicit Vec(const Vec2& v, Real z = 0, Real w = 0)             { x = v.x; y = v.y; this->z = z; this->w = w; }
    /// Construct from 3D vector
    explicit Vec(const Vec3& v, Real w = 1)                         { x = v.x; y = v.y; z = v.z; this->w = w; }
    /// Construct from vector of same dimension
    template<class T>
    Vec(const MatrixBase<T>& rhs)                                   { operator=(rhs); }

    template<class T>
    Vec& operator=(const MatrixBase<T>& rhs)                        { Super::operator=(rhs); return *this; }

    /// \name Specialized for optimization
    /// @{
    Real lengthSqr() const                                          { return x*x + y*y + z*z + w*w; }
    Real dot(const Vec& v) const                                    { return x*v.x + y*v.y + z*v.z + w*v.w; }
    /// @}

    /// 4D vector cross product
    Vec4 cross(const Vec4& v1, const Vec4& v2) const
    {
        Vec4 ret;
        ret.x =     y * (v1.z*v2.w - v2.z*v1.w) - z * (v1.y*v2.w - v2.y*v1.w)   + w * (v1.y*v2.z - v1.z*v2.y);
        ret.y = -(  x * (v1.z*v2.w - v2.z*v1.w) - z * (v1.x*v2.w - v2.x*v1.w)   + w * (v1.x*v2.z - v2.x*v1.z)   );
        ret.z =     x * (v1.y*v2.w - v2.y*v1.w) - y * (v1.x*v2.w - v2.x*v1.w)   + w * (v1.x*v2.y - v2.x*v1.y);
        ret.w = -(  x * (v1.y*v2.z - v2.y*v1.z) - y * (v1.x*v2.z - v2.x*v1.z)   + z * (v1.x*v2.y - v2.x*v1.y)   );
        return ret;
    }

public:
    static const Vec zero;
    static const Vec one;
    static const Vec axisX;
    static const Vec axisY;
    static const Vec axisZ;
    static const Vec axisW;
    static const Vec axis[4];

public:
    /// \name Const swizzle methods
    /// @{
    /*
        //Code Generator

        const int dim = 4;
        char axes[] = {'x','y','z','w'};
        
        for (int i = 0; i < dim; ++i)
        {
            for (int j = 0; j < dim; ++j)
            {
                debug_print(sout() << "VecSwizCon2 " << axes[i] << axes[j] 
                                            << "() const                                              { return VecSwizCon2("
                                            << axes[i] << "," << axes[j] << "); }" << endl);

                for (int k = 0; k < dim; ++k)
                {
                    debug_print(sout() << "VecSwizCon3 " << axes[i] << axes[j] << axes[k] 
                                                << "() const                                             { return VecSwizCon3("
                                                << axes[i] << "," << axes[j] << "," << axes[k] << "); }" << endl);

                    for (int l = 0; l < dim; ++l)
                    {
                        debug_print(sout() << "VecSwizCon4 " << axes[i] << axes[j] << axes[k] << axes[l]
                                                    << "() const                                            { return VecSwizCon4("
                                                    << axes[i] << "," << axes[j] << "," << axes[k] << "," << axes[l] << "); }" << endl);
                    }
                }
            }
        }
    */

    VecSwizCon2 xx() const                                          { return VecSwizCon2(x,x); }
    VecSwizCon3 xxx() const                                         { return VecSwizCon3(x,x,x); }
    VecSwizCon4 xxxx() const                                        { return VecSwizCon4(x,x,x,x); }
    VecSwizCon4 xxxy() const                                        { return VecSwizCon4(x,x,x,y); }
    VecSwizCon4 xxxz() const                                        { return VecSwizCon4(x,x,x,z); }
    VecSwizCon4 xxxw() const                                        { return VecSwizCon4(x,x,x,w); }
    VecSwizCon3 xxy() const                                         { return VecSwizCon3(x,x,y); }
    VecSwizCon4 xxyx() const                                        { return VecSwizCon4(x,x,y,x); }
    VecSwizCon4 xxyy() const                                        { return VecSwizCon4(x,x,y,y); }
    VecSwizCon4 xxyz() const                                        { return VecSwizCon4(x,x,y,z); }
    VecSwizCon4 xxyw() const                                        { return VecSwizCon4(x,x,y,w); }
    VecSwizCon3 xxz() const                                         { return VecSwizCon3(x,x,z); }
    VecSwizCon4 xxzx() const                                        { return VecSwizCon4(x,x,z,x); }
    VecSwizCon4 xxzy() const                                        { return VecSwizCon4(x,x,z,y); }
    VecSwizCon4 xxzz() const                                        { return VecSwizCon4(x,x,z,z); }
    VecSwizCon4 xxzw() const                                        { return VecSwizCon4(x,x,z,w); }
    VecSwizCon3 xxw() const                                         { return VecSwizCon3(x,x,w); }
    VecSwizCon4 xxwx() const                                        { return VecSwizCon4(x,x,w,x); }
    VecSwizCon4 xxwy() const                                        { return VecSwizCon4(x,x,w,y); }
    VecSwizCon4 xxwz() const                                        { return VecSwizCon4(x,x,w,z); }
    VecSwizCon4 xxww() const                                        { return VecSwizCon4(x,x,w,w); }
    VecSwizCon2 xy() const                                          { return VecSwizCon2(x,y); }
    VecSwizCon3 xyx() const                                         { return VecSwizCon3(x,y,x); }
    VecSwizCon4 xyxx() const                                        { return VecSwizCon4(x,y,x,x); }
    VecSwizCon4 xyxy() const                                        { return VecSwizCon4(x,y,x,y); }
    VecSwizCon4 xyxz() const                                        { return VecSwizCon4(x,y,x,z); }
    VecSwizCon4 xyxw() const                                        { return VecSwizCon4(x,y,x,w); }
    VecSwizCon3 xyy() const                                         { return VecSwizCon3(x,y,y); }
    VecSwizCon4 xyyx() const                                        { return VecSwizCon4(x,y,y,x); }
    VecSwizCon4 xyyy() const                                        { return VecSwizCon4(x,y,y,y); }
    VecSwizCon4 xyyz() const                                        { return VecSwizCon4(x,y,y,z); }
    VecSwizCon4 xyyw() const                                        { return VecSwizCon4(x,y,y,w); }
    VecSwizCon3 xyz() const                                         { return VecSwizCon3(x,y,z); }
    VecSwizCon4 xyzx() const                                        { return VecSwizCon4(x,y,z,x); }
    VecSwizCon4 xyzy() const                                        { return VecSwizCon4(x,y,z,y); }
    VecSwizCon4 xyzz() const                                        { return VecSwizCon4(x,y,z,z); }
    VecSwizCon4 xyzw() const                                        { return VecSwizCon4(x,y,z,w); }
    VecSwizCon3 xyw() const                                         { return VecSwizCon3(x,y,w); }
    VecSwizCon4 xywx() const                                        { return VecSwizCon4(x,y,w,x); }
    VecSwizCon4 xywy() const                                        { return VecSwizCon4(x,y,w,y); }
    VecSwizCon4 xywz() const                                        { return VecSwizCon4(x,y,w,z); }
    VecSwizCon4 xyww() const                                        { return VecSwizCon4(x,y,w,w); }
    VecSwizCon2 xz() const                                          { return VecSwizCon2(x,z); }
    VecSwizCon3 xzx() const                                         { return VecSwizCon3(x,z,x); }
    VecSwizCon4 xzxx() const                                        { return VecSwizCon4(x,z,x,x); }
    VecSwizCon4 xzxy() const                                        { return VecSwizCon4(x,z,x,y); }
    VecSwizCon4 xzxz() const                                        { return VecSwizCon4(x,z,x,z); }
    VecSwizCon4 xzxw() const                                        { return VecSwizCon4(x,z,x,w); }
    VecSwizCon3 xzy() const                                         { return VecSwizCon3(x,z,y); }
    VecSwizCon4 xzyx() const                                        { return VecSwizCon4(x,z,y,x); }
    VecSwizCon4 xzyy() const                                        { return VecSwizCon4(x,z,y,y); }
    VecSwizCon4 xzyz() const                                        { return VecSwizCon4(x,z,y,z); }
    VecSwizCon4 xzyw() const                                        { return VecSwizCon4(x,z,y,w); }
    VecSwizCon3 xzz() const                                         { return VecSwizCon3(x,z,z); }
    VecSwizCon4 xzzx() const                                        { return VecSwizCon4(x,z,z,x); }
    VecSwizCon4 xzzy() const                                        { return VecSwizCon4(x,z,z,y); }
    VecSwizCon4 xzzz() const                                        { return VecSwizCon4(x,z,z,z); }
    VecSwizCon4 xzzw() const                                        { return VecSwizCon4(x,z,z,w); }
    VecSwizCon3 xzw() const                                         { return VecSwizCon3(x,z,w); }
    VecSwizCon4 xzwx() const                                        { return VecSwizCon4(x,z,w,x); }
    VecSwizCon4 xzwy() const                                        { return VecSwizCon4(x,z,w,y); }
    VecSwizCon4 xzwz() const                                        { return VecSwizCon4(x,z,w,z); }
    VecSwizCon4 xzww() const                                        { return VecSwizCon4(x,z,w,w); }
    VecSwizCon2 xw() const                                          { return VecSwizCon2(x,w); }
    VecSwizCon3 xwx() const                                         { return VecSwizCon3(x,w,x); }
    VecSwizCon4 xwxx() const                                        { return VecSwizCon4(x,w,x,x); }
    VecSwizCon4 xwxy() const                                        { return VecSwizCon4(x,w,x,y); }
    VecSwizCon4 xwxz() const                                        { return VecSwizCon4(x,w,x,z); }
    VecSwizCon4 xwxw() const                                        { return VecSwizCon4(x,w,x,w); }
    VecSwizCon3 xwy() const                                         { return VecSwizCon3(x,w,y); }
    VecSwizCon4 xwyx() const                                        { return VecSwizCon4(x,w,y,x); }
    VecSwizCon4 xwyy() const                                        { return VecSwizCon4(x,w,y,y); }
    VecSwizCon4 xwyz() const                                        { return VecSwizCon4(x,w,y,z); }
    VecSwizCon4 xwyw() const                                        { return VecSwizCon4(x,w,y,w); }
    VecSwizCon3 xwz() const                                         { return VecSwizCon3(x,w,z); }
    VecSwizCon4 xwzx() const                                        { return VecSwizCon4(x,w,z,x); }
    VecSwizCon4 xwzy() const                                        { return VecSwizCon4(x,w,z,y); }
    VecSwizCon4 xwzz() const                                        { return VecSwizCon4(x,w,z,z); }
    VecSwizCon4 xwzw() const                                        { return VecSwizCon4(x,w,z,w); }
    VecSwizCon3 xww() const                                         { return VecSwizCon3(x,w,w); }
    VecSwizCon4 xwwx() const                                        { return VecSwizCon4(x,w,w,x); }
    VecSwizCon4 xwwy() const                                        { return VecSwizCon4(x,w,w,y); }
    VecSwizCon4 xwwz() const                                        { return VecSwizCon4(x,w,w,z); }
    VecSwizCon4 xwww() const                                        { return VecSwizCon4(x,w,w,w); }
    VecSwizCon2 yx() const                                          { return VecSwizCon2(y,x); }
    VecSwizCon3 yxx() const                                         { return VecSwizCon3(y,x,x); }
    VecSwizCon4 yxxx() const                                        { return VecSwizCon4(y,x,x,x); }
    VecSwizCon4 yxxy() const                                        { return VecSwizCon4(y,x,x,y); }
    VecSwizCon4 yxxz() const                                        { return VecSwizCon4(y,x,x,z); }
    VecSwizCon4 yxxw() const                                        { return VecSwizCon4(y,x,x,w); }
    VecSwizCon3 yxy() const                                         { return VecSwizCon3(y,x,y); }
    VecSwizCon4 yxyx() const                                        { return VecSwizCon4(y,x,y,x); }
    VecSwizCon4 yxyy() const                                        { return VecSwizCon4(y,x,y,y); }
    VecSwizCon4 yxyz() const                                        { return VecSwizCon4(y,x,y,z); }
    VecSwizCon4 yxyw() const                                        { return VecSwizCon4(y,x,y,w); }
    VecSwizCon3 yxz() const                                         { return VecSwizCon3(y,x,z); }
    VecSwizCon4 yxzx() const                                        { return VecSwizCon4(y,x,z,x); }
    VecSwizCon4 yxzy() const                                        { return VecSwizCon4(y,x,z,y); }
    VecSwizCon4 yxzz() const                                        { return VecSwizCon4(y,x,z,z); }
    VecSwizCon4 yxzw() const                                        { return VecSwizCon4(y,x,z,w); }
    VecSwizCon3 yxw() const                                         { return VecSwizCon3(y,x,w); }
    VecSwizCon4 yxwx() const                                        { return VecSwizCon4(y,x,w,x); }
    VecSwizCon4 yxwy() const                                        { return VecSwizCon4(y,x,w,y); }
    VecSwizCon4 yxwz() const                                        { return VecSwizCon4(y,x,w,z); }
    VecSwizCon4 yxww() const                                        { return VecSwizCon4(y,x,w,w); }
    VecSwizCon2 yy() const                                          { return VecSwizCon2(y,y); }
    VecSwizCon3 yyx() const                                         { return VecSwizCon3(y,y,x); }
    VecSwizCon4 yyxx() const                                        { return VecSwizCon4(y,y,x,x); }
    VecSwizCon4 yyxy() const                                        { return VecSwizCon4(y,y,x,y); }
    VecSwizCon4 yyxz() const                                        { return VecSwizCon4(y,y,x,z); }
    VecSwizCon4 yyxw() const                                        { return VecSwizCon4(y,y,x,w); }
    VecSwizCon3 yyy() const                                         { return VecSwizCon3(y,y,y); }
    VecSwizCon4 yyyx() const                                        { return VecSwizCon4(y,y,y,x); }
    VecSwizCon4 yyyy() const                                        { return VecSwizCon4(y,y,y,y); }
    VecSwizCon4 yyyz() const                                        { return VecSwizCon4(y,y,y,z); }
    VecSwizCon4 yyyw() const                                        { return VecSwizCon4(y,y,y,w); }
    VecSwizCon3 yyz() const                                         { return VecSwizCon3(y,y,z); }
    VecSwizCon4 yyzx() const                                        { return VecSwizCon4(y,y,z,x); }
    VecSwizCon4 yyzy() const                                        { return VecSwizCon4(y,y,z,y); }
    VecSwizCon4 yyzz() const                                        { return VecSwizCon4(y,y,z,z); }
    VecSwizCon4 yyzw() const                                        { return VecSwizCon4(y,y,z,w); }
    VecSwizCon3 yyw() const                                         { return VecSwizCon3(y,y,w); }
    VecSwizCon4 yywx() const                                        { return VecSwizCon4(y,y,w,x); }
    VecSwizCon4 yywy() const                                        { return VecSwizCon4(y,y,w,y); }
    VecSwizCon4 yywz() const                                        { return VecSwizCon4(y,y,w,z); }
    VecSwizCon4 yyww() const                                        { return VecSwizCon4(y,y,w,w); }
    VecSwizCon2 yz() const                                          { return VecSwizCon2(y,z); }
    VecSwizCon3 yzx() const                                         { return VecSwizCon3(y,z,x); }
    VecSwizCon4 yzxx() const                                        { return VecSwizCon4(y,z,x,x); }
    VecSwizCon4 yzxy() const                                        { return VecSwizCon4(y,z,x,y); }
    VecSwizCon4 yzxz() const                                        { return VecSwizCon4(y,z,x,z); }
    VecSwizCon4 yzxw() const                                        { return VecSwizCon4(y,z,x,w); }
    VecSwizCon3 yzy() const                                         { return VecSwizCon3(y,z,y); }
    VecSwizCon4 yzyx() const                                        { return VecSwizCon4(y,z,y,x); }
    VecSwizCon4 yzyy() const                                        { return VecSwizCon4(y,z,y,y); }
    VecSwizCon4 yzyz() const                                        { return VecSwizCon4(y,z,y,z); }
    VecSwizCon4 yzyw() const                                        { return VecSwizCon4(y,z,y,w); }
    VecSwizCon3 yzz() const                                         { return VecSwizCon3(y,z,z); }
    VecSwizCon4 yzzx() const                                        { return VecSwizCon4(y,z,z,x); }
    VecSwizCon4 yzzy() const                                        { return VecSwizCon4(y,z,z,y); }
    VecSwizCon4 yzzz() const                                        { return VecSwizCon4(y,z,z,z); }
    VecSwizCon4 yzzw() const                                        { return VecSwizCon4(y,z,z,w); }
    VecSwizCon3 yzw() const                                         { return VecSwizCon3(y,z,w); }
    VecSwizCon4 yzwx() const                                        { return VecSwizCon4(y,z,w,x); }
    VecSwizCon4 yzwy() const                                        { return VecSwizCon4(y,z,w,y); }
    VecSwizCon4 yzwz() const                                        { return VecSwizCon4(y,z,w,z); }
    VecSwizCon4 yzww() const                                        { return VecSwizCon4(y,z,w,w); }
    VecSwizCon2 yw() const                                          { return VecSwizCon2(y,w); }
    VecSwizCon3 ywx() const                                         { return VecSwizCon3(y,w,x); }
    VecSwizCon4 ywxx() const                                        { return VecSwizCon4(y,w,x,x); }
    VecSwizCon4 ywxy() const                                        { return VecSwizCon4(y,w,x,y); }
    VecSwizCon4 ywxz() const                                        { return VecSwizCon4(y,w,x,z); }
    VecSwizCon4 ywxw() const                                        { return VecSwizCon4(y,w,x,w); }
    VecSwizCon3 ywy() const                                         { return VecSwizCon3(y,w,y); }
    VecSwizCon4 ywyx() const                                        { return VecSwizCon4(y,w,y,x); }
    VecSwizCon4 ywyy() const                                        { return VecSwizCon4(y,w,y,y); }
    VecSwizCon4 ywyz() const                                        { return VecSwizCon4(y,w,y,z); }
    VecSwizCon4 ywyw() const                                        { return VecSwizCon4(y,w,y,w); }
    VecSwizCon3 ywz() const                                         { return VecSwizCon3(y,w,z); }
    VecSwizCon4 ywzx() const                                        { return VecSwizCon4(y,w,z,x); }
    VecSwizCon4 ywzy() const                                        { return VecSwizCon4(y,w,z,y); }
    VecSwizCon4 ywzz() const                                        { return VecSwizCon4(y,w,z,z); }
    VecSwizCon4 ywzw() const                                        { return VecSwizCon4(y,w,z,w); }
    VecSwizCon3 yww() const                                         { return VecSwizCon3(y,w,w); }
    VecSwizCon4 ywwx() const                                        { return VecSwizCon4(y,w,w,x); }
    VecSwizCon4 ywwy() const                                        { return VecSwizCon4(y,w,w,y); }
    VecSwizCon4 ywwz() const                                        { return VecSwizCon4(y,w,w,z); }
    VecSwizCon4 ywww() const                                        { return VecSwizCon4(y,w,w,w); }
    VecSwizCon2 zx() const                                          { return VecSwizCon2(z,x); }
    VecSwizCon3 zxx() const                                         { return VecSwizCon3(z,x,x); }
    VecSwizCon4 zxxx() const                                        { return VecSwizCon4(z,x,x,x); }
    VecSwizCon4 zxxy() const                                        { return VecSwizCon4(z,x,x,y); }
    VecSwizCon4 zxxz() const                                        { return VecSwizCon4(z,x,x,z); }
    VecSwizCon4 zxxw() const                                        { return VecSwizCon4(z,x,x,w); }
    VecSwizCon3 zxy() const                                         { return VecSwizCon3(z,x,y); }
    VecSwizCon4 zxyx() const                                        { return VecSwizCon4(z,x,y,x); }
    VecSwizCon4 zxyy() const                                        { return VecSwizCon4(z,x,y,y); }
    VecSwizCon4 zxyz() const                                        { return VecSwizCon4(z,x,y,z); }
    VecSwizCon4 zxyw() const                                        { return VecSwizCon4(z,x,y,w); }
    VecSwizCon3 zxz() const                                         { return VecSwizCon3(z,x,z); }
    VecSwizCon4 zxzx() const                                        { return VecSwizCon4(z,x,z,x); }
    VecSwizCon4 zxzy() const                                        { return VecSwizCon4(z,x,z,y); }
    VecSwizCon4 zxzz() const                                        { return VecSwizCon4(z,x,z,z); }
    VecSwizCon4 zxzw() const                                        { return VecSwizCon4(z,x,z,w); }
    VecSwizCon3 zxw() const                                         { return VecSwizCon3(z,x,w); }
    VecSwizCon4 zxwx() const                                        { return VecSwizCon4(z,x,w,x); }
    VecSwizCon4 zxwy() const                                        { return VecSwizCon4(z,x,w,y); }
    VecSwizCon4 zxwz() const                                        { return VecSwizCon4(z,x,w,z); }
    VecSwizCon4 zxww() const                                        { return VecSwizCon4(z,x,w,w); }
    VecSwizCon2 zy() const                                          { return VecSwizCon2(z,y); }
    VecSwizCon3 zyx() const                                         { return VecSwizCon3(z,y,x); }
    VecSwizCon4 zyxx() const                                        { return VecSwizCon4(z,y,x,x); }
    VecSwizCon4 zyxy() const                                        { return VecSwizCon4(z,y,x,y); }
    VecSwizCon4 zyxz() const                                        { return VecSwizCon4(z,y,x,z); }
    VecSwizCon4 zyxw() const                                        { return VecSwizCon4(z,y,x,w); }
    VecSwizCon3 zyy() const                                         { return VecSwizCon3(z,y,y); }
    VecSwizCon4 zyyx() const                                        { return VecSwizCon4(z,y,y,x); }
    VecSwizCon4 zyyy() const                                        { return VecSwizCon4(z,y,y,y); }
    VecSwizCon4 zyyz() const                                        { return VecSwizCon4(z,y,y,z); }
    VecSwizCon4 zyyw() const                                        { return VecSwizCon4(z,y,y,w); }
    VecSwizCon3 zyz() const                                         { return VecSwizCon3(z,y,z); }
    VecSwizCon4 zyzx() const                                        { return VecSwizCon4(z,y,z,x); }
    VecSwizCon4 zyzy() const                                        { return VecSwizCon4(z,y,z,y); }
    VecSwizCon4 zyzz() const                                        { return VecSwizCon4(z,y,z,z); }
    VecSwizCon4 zyzw() const                                        { return VecSwizCon4(z,y,z,w); }
    VecSwizCon3 zyw() const                                         { return VecSwizCon3(z,y,w); }
    VecSwizCon4 zywx() const                                        { return VecSwizCon4(z,y,w,x); }
    VecSwizCon4 zywy() const                                        { return VecSwizCon4(z,y,w,y); }
    VecSwizCon4 zywz() const                                        { return VecSwizCon4(z,y,w,z); }
    VecSwizCon4 zyww() const                                        { return VecSwizCon4(z,y,w,w); }
    VecSwizCon2 zz() const                                          { return VecSwizCon2(z,z); }
    VecSwizCon3 zzx() const                                         { return VecSwizCon3(z,z,x); }
    VecSwizCon4 zzxx() const                                        { return VecSwizCon4(z,z,x,x); }
    VecSwizCon4 zzxy() const                                        { return VecSwizCon4(z,z,x,y); }
    VecSwizCon4 zzxz() const                                        { return VecSwizCon4(z,z,x,z); }
    VecSwizCon4 zzxw() const                                        { return VecSwizCon4(z,z,x,w); }
    VecSwizCon3 zzy() const                                         { return VecSwizCon3(z,z,y); }
    VecSwizCon4 zzyx() const                                        { return VecSwizCon4(z,z,y,x); }
    VecSwizCon4 zzyy() const                                        { return VecSwizCon4(z,z,y,y); }
    VecSwizCon4 zzyz() const                                        { return VecSwizCon4(z,z,y,z); }
    VecSwizCon4 zzyw() const                                        { return VecSwizCon4(z,z,y,w); }
    VecSwizCon3 zzz() const                                         { return VecSwizCon3(z,z,z); }
    VecSwizCon4 zzzx() const                                        { return VecSwizCon4(z,z,z,x); }
    VecSwizCon4 zzzy() const                                        { return VecSwizCon4(z,z,z,y); }
    VecSwizCon4 zzzz() const                                        { return VecSwizCon4(z,z,z,z); }
    VecSwizCon4 zzzw() const                                        { return VecSwizCon4(z,z,z,w); }
    VecSwizCon3 zzw() const                                         { return VecSwizCon3(z,z,w); }
    VecSwizCon4 zzwx() const                                        { return VecSwizCon4(z,z,w,x); }
    VecSwizCon4 zzwy() const                                        { return VecSwizCon4(z,z,w,y); }
    VecSwizCon4 zzwz() const                                        { return VecSwizCon4(z,z,w,z); }
    VecSwizCon4 zzww() const                                        { return VecSwizCon4(z,z,w,w); }
    VecSwizCon2 zw() const                                          { return VecSwizCon2(z,w); }
    VecSwizCon3 zwx() const                                         { return VecSwizCon3(z,w,x); }
    VecSwizCon4 zwxx() const                                        { return VecSwizCon4(z,w,x,x); }
    VecSwizCon4 zwxy() const                                        { return VecSwizCon4(z,w,x,y); }
    VecSwizCon4 zwxz() const                                        { return VecSwizCon4(z,w,x,z); }
    VecSwizCon4 zwxw() const                                        { return VecSwizCon4(z,w,x,w); }
    VecSwizCon3 zwy() const                                         { return VecSwizCon3(z,w,y); }
    VecSwizCon4 zwyx() const                                        { return VecSwizCon4(z,w,y,x); }
    VecSwizCon4 zwyy() const                                        { return VecSwizCon4(z,w,y,y); }
    VecSwizCon4 zwyz() const                                        { return VecSwizCon4(z,w,y,z); }
    VecSwizCon4 zwyw() const                                        { return VecSwizCon4(z,w,y,w); }
    VecSwizCon3 zwz() const                                         { return VecSwizCon3(z,w,z); }
    VecSwizCon4 zwzx() const                                        { return VecSwizCon4(z,w,z,x); }
    VecSwizCon4 zwzy() const                                        { return VecSwizCon4(z,w,z,y); }
    VecSwizCon4 zwzz() const                                        { return VecSwizCon4(z,w,z,z); }
    VecSwizCon4 zwzw() const                                        { return VecSwizCon4(z,w,z,w); }
    VecSwizCon3 zww() const                                         { return VecSwizCon3(z,w,w); }
    VecSwizCon4 zwwx() const                                        { return VecSwizCon4(z,w,w,x); }
    VecSwizCon4 zwwy() const                                        { return VecSwizCon4(z,w,w,y); }
    VecSwizCon4 zwwz() const                                        { return VecSwizCon4(z,w,w,z); }
    VecSwizCon4 zwww() const                                        { return VecSwizCon4(z,w,w,w); }
    VecSwizCon2 wx() const                                          { return VecSwizCon2(w,x); }
    VecSwizCon3 wxx() const                                         { return VecSwizCon3(w,x,x); }
    VecSwizCon4 wxxx() const                                        { return VecSwizCon4(w,x,x,x); }
    VecSwizCon4 wxxy() const                                        { return VecSwizCon4(w,x,x,y); }
    VecSwizCon4 wxxz() const                                        { return VecSwizCon4(w,x,x,z); }
    VecSwizCon4 wxxw() const                                        { return VecSwizCon4(w,x,x,w); }
    VecSwizCon3 wxy() const                                         { return VecSwizCon3(w,x,y); }
    VecSwizCon4 wxyx() const                                        { return VecSwizCon4(w,x,y,x); }
    VecSwizCon4 wxyy() const                                        { return VecSwizCon4(w,x,y,y); }
    VecSwizCon4 wxyz() const                                        { return VecSwizCon4(w,x,y,z); }
    VecSwizCon4 wxyw() const                                        { return VecSwizCon4(w,x,y,w); }
    VecSwizCon3 wxz() const                                         { return VecSwizCon3(w,x,z); }
    VecSwizCon4 wxzx() const                                        { return VecSwizCon4(w,x,z,x); }
    VecSwizCon4 wxzy() const                                        { return VecSwizCon4(w,x,z,y); }
    VecSwizCon4 wxzz() const                                        { return VecSwizCon4(w,x,z,z); }
    VecSwizCon4 wxzw() const                                        { return VecSwizCon4(w,x,z,w); }
    VecSwizCon3 wxw() const                                         { return VecSwizCon3(w,x,w); }
    VecSwizCon4 wxwx() const                                        { return VecSwizCon4(w,x,w,x); }
    VecSwizCon4 wxwy() const                                        { return VecSwizCon4(w,x,w,y); }
    VecSwizCon4 wxwz() const                                        { return VecSwizCon4(w,x,w,z); }
    VecSwizCon4 wxww() const                                        { return VecSwizCon4(w,x,w,w); }
    VecSwizCon2 wy() const                                          { return VecSwizCon2(w,y); }
    VecSwizCon3 wyx() const                                         { return VecSwizCon3(w,y,x); }
    VecSwizCon4 wyxx() const                                        { return VecSwizCon4(w,y,x,x); }
    VecSwizCon4 wyxy() const                                        { return VecSwizCon4(w,y,x,y); }
    VecSwizCon4 wyxz() const                                        { return VecSwizCon4(w,y,x,z); }
    VecSwizCon4 wyxw() const                                        { return VecSwizCon4(w,y,x,w); }
    VecSwizCon3 wyy() const                                         { return VecSwizCon3(w,y,y); }
    VecSwizCon4 wyyx() const                                        { return VecSwizCon4(w,y,y,x); }
    VecSwizCon4 wyyy() const                                        { return VecSwizCon4(w,y,y,y); }
    VecSwizCon4 wyyz() const                                        { return VecSwizCon4(w,y,y,z); }
    VecSwizCon4 wyyw() const                                        { return VecSwizCon4(w,y,y,w); }
    VecSwizCon3 wyz() const                                         { return VecSwizCon3(w,y,z); }
    VecSwizCon4 wyzx() const                                        { return VecSwizCon4(w,y,z,x); }
    VecSwizCon4 wyzy() const                                        { return VecSwizCon4(w,y,z,y); }
    VecSwizCon4 wyzz() const                                        { return VecSwizCon4(w,y,z,z); }
    VecSwizCon4 wyzw() const                                        { return VecSwizCon4(w,y,z,w); }
    VecSwizCon3 wyw() const                                         { return VecSwizCon3(w,y,w); }
    VecSwizCon4 wywx() const                                        { return VecSwizCon4(w,y,w,x); }
    VecSwizCon4 wywy() const                                        { return VecSwizCon4(w,y,w,y); }
    VecSwizCon4 wywz() const                                        { return VecSwizCon4(w,y,w,z); }
    VecSwizCon4 wyww() const                                        { return VecSwizCon4(w,y,w,w); }
    VecSwizCon2 wz() const                                          { return VecSwizCon2(w,z); }
    VecSwizCon3 wzx() const                                         { return VecSwizCon3(w,z,x); }
    VecSwizCon4 wzxx() const                                        { return VecSwizCon4(w,z,x,x); }
    VecSwizCon4 wzxy() const                                        { return VecSwizCon4(w,z,x,y); }
    VecSwizCon4 wzxz() const                                        { return VecSwizCon4(w,z,x,z); }
    VecSwizCon4 wzxw() const                                        { return VecSwizCon4(w,z,x,w); }
    VecSwizCon3 wzy() const                                         { return VecSwizCon3(w,z,y); }
    VecSwizCon4 wzyx() const                                        { return VecSwizCon4(w,z,y,x); }
    VecSwizCon4 wzyy() const                                        { return VecSwizCon4(w,z,y,y); }
    VecSwizCon4 wzyz() const                                        { return VecSwizCon4(w,z,y,z); }
    VecSwizCon4 wzyw() const                                        { return VecSwizCon4(w,z,y,w); }
    VecSwizCon3 wzz() const                                         { return VecSwizCon3(w,z,z); }
    VecSwizCon4 wzzx() const                                        { return VecSwizCon4(w,z,z,x); }
    VecSwizCon4 wzzy() const                                        { return VecSwizCon4(w,z,z,y); }
    VecSwizCon4 wzzz() const                                        { return VecSwizCon4(w,z,z,z); }
    VecSwizCon4 wzzw() const                                        { return VecSwizCon4(w,z,z,w); }
    VecSwizCon3 wzw() const                                         { return VecSwizCon3(w,z,w); }
    VecSwizCon4 wzwx() const                                        { return VecSwizCon4(w,z,w,x); }
    VecSwizCon4 wzwy() const                                        { return VecSwizCon4(w,z,w,y); }
    VecSwizCon4 wzwz() const                                        { return VecSwizCon4(w,z,w,z); }
    VecSwizCon4 wzww() const                                        { return VecSwizCon4(w,z,w,w); }
    VecSwizCon2 ww() const                                          { return VecSwizCon2(w,w); }
    VecSwizCon3 wwx() const                                         { return VecSwizCon3(w,w,x); }
    VecSwizCon4 wwxx() const                                        { return VecSwizCon4(w,w,x,x); }
    VecSwizCon4 wwxy() const                                        { return VecSwizCon4(w,w,x,y); }
    VecSwizCon4 wwxz() const                                        { return VecSwizCon4(w,w,x,z); }
    VecSwizCon4 wwxw() const                                        { return VecSwizCon4(w,w,x,w); }
    VecSwizCon3 wwy() const                                         { return VecSwizCon3(w,w,y); }
    VecSwizCon4 wwyx() const                                        { return VecSwizCon4(w,w,y,x); }
    VecSwizCon4 wwyy() const                                        { return VecSwizCon4(w,w,y,y); }
    VecSwizCon4 wwyz() const                                        { return VecSwizCon4(w,w,y,z); }
    VecSwizCon4 wwyw() const                                        { return VecSwizCon4(w,w,y,w); }
    VecSwizCon3 wwz() const                                         { return VecSwizCon3(w,w,z); }
    VecSwizCon4 wwzx() const                                        { return VecSwizCon4(w,w,z,x); }
    VecSwizCon4 wwzy() const                                        { return VecSwizCon4(w,w,z,y); }
    VecSwizCon4 wwzz() const                                        { return VecSwizCon4(w,w,z,z); }
    VecSwizCon4 wwzw() const                                        { return VecSwizCon4(w,w,z,w); }
    VecSwizCon3 www() const                                         { return VecSwizCon3(w,w,w); }
    VecSwizCon4 wwwx() const                                        { return VecSwizCon4(w,w,w,x); }
    VecSwizCon4 wwwy() const                                        { return VecSwizCon4(w,w,w,y); }
    VecSwizCon4 wwwz() const                                        { return VecSwizCon4(w,w,w,z); }
    VecSwizCon4 wwww() const                                        { return VecSwizCon4(w,w,w,w); }
    /// @}

    /// \name Mutable swizzle methods
    /// @{
    /*
        //Code Generator

        const int dim = 4;
        char axes[] = {'x','y','z','w'};
        bool used[] = {false, false, false, false};

        for (int i = 0; i < dim; ++i)
        {
            used[i] = true;
            for (int j = 0; j < dim; ++j)
            {
                if (used[j]) continue;
                used[j] = true;

                debug_print(sout() << "VecSwizRef2 " << axes[i] << axes[j] 
                                            << "()                                                    { return VecSwizRef2("
                                            << axes[i] << "," << axes[j] << "); }" << endl);

                for (int k = 0; k < dim; ++k)
                {
                    if (used[k]) continue;
                    used[k] = true;

                    debug_print(sout() << "VecSwizRef3 " << axes[i] << axes[j] << axes[k] 
                                                << "()                                                   { return VecSwizRef3("
                                                << axes[i] << "," << axes[j] << "," << axes[k] << "); }" << endl);

                    for (int l = 0; l < dim; ++l)
                    {
                        if (used[l]) continue;
                        debug_print(sout() << "VecSwizRef4 " << axes[i] << axes[j] << axes[k] << axes[l]
                                                    << "()                                                  { return VecSwizRef4("
                                                    << axes[i] << "," << axes[j] << "," << axes[k] << "," << axes[l] << "); }" << endl);
                    }

                    used[k] = false;
                }
                used[j] = false;
            }
            used[i] = false;
        }
    */

    VecSwizRef2 xy()                                                { return VecSwizRef2(x,y); }
    VecSwizRef3 xyz()                                               { return VecSwizRef3(x,y,z); }
    VecSwizRef4 xyzw()                                              { return VecSwizRef4(x,y,z,w); }
    VecSwizRef3 xyw()                                               { return VecSwizRef3(x,y,w); }
    VecSwizRef4 xywz()                                              { return VecSwizRef4(x,y,w,z); }
    VecSwizRef2 xz()                                                { return VecSwizRef2(x,z); }
    VecSwizRef3 xzy()                                               { return VecSwizRef3(x,z,y); }
    VecSwizRef4 xzyw()                                              { return VecSwizRef4(x,z,y,w); }
    VecSwizRef3 xzw()                                               { return VecSwizRef3(x,z,w); }
    VecSwizRef4 xzwy()                                              { return VecSwizRef4(x,z,w,y); }
    VecSwizRef2 xw()                                                { return VecSwizRef2(x,w); }
    VecSwizRef3 xwy()                                               { return VecSwizRef3(x,w,y); }
    VecSwizRef4 xwyz()                                              { return VecSwizRef4(x,w,y,z); }
    VecSwizRef3 xwz()                                               { return VecSwizRef3(x,w,z); }
    VecSwizRef4 xwzy()                                              { return VecSwizRef4(x,w,z,y); }
    VecSwizRef2 yx()                                                { return VecSwizRef2(y,x); }
    VecSwizRef3 yxz()                                               { return VecSwizRef3(y,x,z); }
    VecSwizRef4 yxzw()                                              { return VecSwizRef4(y,x,z,w); }
    VecSwizRef3 yxw()                                               { return VecSwizRef3(y,x,w); }
    VecSwizRef4 yxwz()                                              { return VecSwizRef4(y,x,w,z); }
    VecSwizRef2 yz()                                                { return VecSwizRef2(y,z); }
    VecSwizRef3 yzx()                                               { return VecSwizRef3(y,z,x); }
    VecSwizRef4 yzxw()                                              { return VecSwizRef4(y,z,x,w); }
    VecSwizRef3 yzw()                                               { return VecSwizRef3(y,z,w); }
    VecSwizRef4 yzwx()                                              { return VecSwizRef4(y,z,w,x); }
    VecSwizRef2 yw()                                                { return VecSwizRef2(y,w); }
    VecSwizRef3 ywx()                                               { return VecSwizRef3(y,w,x); }
    VecSwizRef4 ywxz()                                              { return VecSwizRef4(y,w,x,z); }
    VecSwizRef3 ywz()                                               { return VecSwizRef3(y,w,z); }
    VecSwizRef4 ywzx()                                              { return VecSwizRef4(y,w,z,x); }
    VecSwizRef2 zx()                                                { return VecSwizRef2(z,x); }
    VecSwizRef3 zxy()                                               { return VecSwizRef3(z,x,y); }
    VecSwizRef4 zxyw()                                              { return VecSwizRef4(z,x,y,w); }
    VecSwizRef3 zxw()                                               { return VecSwizRef3(z,x,w); }
    VecSwizRef4 zxwy()                                              { return VecSwizRef4(z,x,w,y); }
    VecSwizRef2 zy()                                                { return VecSwizRef2(z,y); }
    VecSwizRef3 zyx()                                               { return VecSwizRef3(z,y,x); }
    VecSwizRef4 zyxw()                                              { return VecSwizRef4(z,y,x,w); }
    VecSwizRef3 zyw()                                               { return VecSwizRef3(z,y,w); }
    VecSwizRef4 zywx()                                              { return VecSwizRef4(z,y,w,x); }
    VecSwizRef2 zw()                                                { return VecSwizRef2(z,w); }
    VecSwizRef3 zwx()                                               { return VecSwizRef3(z,w,x); }
    VecSwizRef4 zwxy()                                              { return VecSwizRef4(z,w,x,y); }
    VecSwizRef3 zwy()                                               { return VecSwizRef3(z,w,y); }
    VecSwizRef4 zwyx()                                              { return VecSwizRef4(z,w,y,x); }
    VecSwizRef2 wx()                                                { return VecSwizRef2(w,x); }
    VecSwizRef3 wxy()                                               { return VecSwizRef3(w,x,y); }
    VecSwizRef4 wxyz()                                              { return VecSwizRef4(w,x,y,z); }
    VecSwizRef3 wxz()                                               { return VecSwizRef3(w,x,z); }
    VecSwizRef4 wxzy()                                              { return VecSwizRef4(w,x,z,y); }
    VecSwizRef2 wy()                                                { return VecSwizRef2(w,y); }
    VecSwizRef3 wyx()                                               { return VecSwizRef3(w,y,x); }
    VecSwizRef4 wyxz()                                              { return VecSwizRef4(w,y,x,z); }
    VecSwizRef3 wyz()                                               { return VecSwizRef3(w,y,z); }
    VecSwizRef4 wyzx()                                              { return VecSwizRef4(w,y,z,x); }
    VecSwizRef2 wz()                                                { return VecSwizRef2(w,z); }
    VecSwizRef3 wzx()                                               { return VecSwizRef3(w,z,x); }
    VecSwizRef4 wzxy()                                              { return VecSwizRef4(w,z,x,y); }
    VecSwizRef3 wzy()                                               { return VecSwizRef3(w,z,y); }
    VecSwizRef4 wzyx()                                              { return VecSwizRef4(w,z,y,x); }
    /// @}
};

template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::zero          (0);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::one           (1);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::axisX         (1, 0, 0, 0);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::axisY         (0, 1, 0, 0);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::axisZ         (0, 0, 1, 0);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::axisW         (0, 0, 0, 1);
template<class R, int O> const Vec<4,R,O> Vec<4,R,O>::axis[4]       = { axisX, axisY, axisZ, axisW };

/** \cond */
/// \name Specialized for optimization
/// @{
template<class R, int Opt>
struct priv::map_impl<Vec<4,R,Opt>, Vec<4,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& v, O&& o, Func&& f)                         { o.x = f(v.x); o.y = f(v.y); o.z = f(v.z); o.w = f(v.w); return forward<O>(o); }
};

template<class R, int Opt>
struct priv::map_impl<Vec<4,R,Opt>, Vec<4,R,Opt>, Vec<4,R,Opt>>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& v, T2&& rhs, O&& o, Func&& f)               { o.x = f(v.x,rhs.x); o.y = f(v.y,rhs.y); o.z = f(v.z,rhs.z); o.w = f(v.w,rhs.w); return forward<O>(o); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<4,R,O>, Accum_>
{
    template<class T, class Accum, class Func>
    static Accum_ func(T&& v, Accum&& initVal, Func&& f)            { return f(f(f(f(forward<Accum>(initVal), v.x), v.y), v.z), v.w); }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Vec<4,R,O>, Accum_, Vec<4,R,O>>
{
    template<class T, class T2, class Accum, class Func>
    static Accum_ func(T&& v, T2&& rhs, Accum&& initVal, Func&& f)  { return f(f(f(f(forward<Accum>(initVal), v.x, rhs.x), v.y, rhs.y), v.z, rhs.z), v.w, rhs.w); }
};
/// @}
/** \endcond */

/// 4D const swizzle vector
template<class Real, int Options>
class VecSwizCon<4,Real,Options> : public VecSwizConBase<VecSwizCon<4,Real,Options>>
{
public:
    VecSwizCon(Real x, Real y, Real z, Real w)                      { this->x = x; this->y = y; this->z = z; this->w = w; }
};

/// 4D mutable swizzle vector
template<class Real, int Options>
class VecSwizRef<4,Real,Options> : public VecSwizRefBase<VecSwizRef<4,Real,Options>>
{
    template<class> friend class VecSwizRefBase;
public:
    typedef VecSwizRefBase<VecSwizRef<4,Real,Options>> Super;
    using Super::operator=;
    using Super::x;
    using Super::y;
    using Super::z;
    using Super::w;
    
    VecSwizRef(Real& x, Real& y, Real& z, Real& w)                  : rx(x), ry(y), rz(z), rw(w) { this->x = x; this->y = y; this->z = z; this->w = w; }

    VecSwizRef& commit()                                            { rx = x; ry = y; rz = z; rw = w; return *this; }

private:
    /// Reference to swizzled vector
    Real& rx;
    Real& ry;
    Real& rz;
    Real& rw;
};


/// 4D column vector types
typedef Vec<4>                                  Vec4;
typedef Vec<4,Float>                            Vec4_f;
typedef Vec<4,Double>                           Vec4_d;

/// 4D row vector types
typedef Vec<4,Real,matrix::Option::vecRow>      VecRow4;
typedef Vec<4,Float,matrix::Option::vecRow>     VecRow4_f;
typedef Vec<4,Double,matrix::Option::vecRow>    VecRow4_d;

}

#include "Honey/Math/Alge/Vec/platform/Vec4.h"
