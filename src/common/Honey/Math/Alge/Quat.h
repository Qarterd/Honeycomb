// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Vec3.h"
#include "Honey/Math/Alge/Vec/Vec4.h"

namespace honey
{

template<class Real> class Interp_;

/// Quaternion rotation class.  Represents a counter-clockwise rotation of an angle about its axis
/**
  * Quaternion concatenation, like matrices, follows a right-to-left ordering.
  *
  * ie. To rotate a vector first by `q0`, followed by a rotation of `q1`, apply \f$ v' = q_1*q_0*v \f$
  */
template<class Real__>
class Quat_
{
public:
    typedef Real__ Real;
private:
    typedef typename Numeral<Real>::Real_ Real_;
    typedef Alge_<Real>         Alge;
    typedef Trig_<Real>         Trig;
    typedef Vec<3,Real>         Vec3;
    typedef Vec<4,Real>         Vec4;
    typedef Matrix<4,4,Real>    Matrix4;
    typedef Interp_<Real>       Interp;

    /// Euler order options
    enum
    {
        eulAxX = 0,
        eulAxY = 1,
        eulAxZ = 2,
        eulFrmS = 0,
        eulFrmR = 1,
        eulRepNo = 0,
        eulRepYes = 1,
        eulParEven = 0,
        eulParOdd = 1
    };
    
public:
    static const int dim = 4;

    //  Creates an order value between 0 and 23 from 4-tuple choices
    /*
      * There are 24 possible conventions, designated by:
      * - eulAxI, axis used initially
      * - eulPar, parity of axis permutation
      * - eulRep, repetition of initial axis as last
      * - eulFrm, frame from which axes are taken
      *
      * Axes I,J,K will be a permutation of X,Y,Z.
      * Axis H will be either I or K, depending on eulRep.
      * Frame S takes axes from initial static frame.
      * If ord = (AxI=X, Par=Even, Rep=No, Frm=S), then {a,b,c,ord} means Rz(c)Ry(b)Rx(a),
      * where Rz(c)v rotates v around Z by c radians.
      *
      * Ken Shoemake, 1993
      */
    #define _ORD(i,p,r,f) (((((((i)<<1)+(p))<<1)+(r))<<1)+(f))
    
    /// Euler angle order
    /** 
      * The default order `xyz_s` represents a rotation of z radians around the z-axis,
      * followed by a rotation around the y-axis then x-axis. All axes are from the static (initial) frame.
      *
      * \retval (axes)_s    Static (initial) frame axes
      * \retval (axes)_r    Rotating frame axes
      */
    enum class EulerOrder
    {
        xyz_s = _ORD(eulAxX, eulParEven,  eulRepNo,   eulFrmS),
        xyx_s = _ORD(eulAxX, eulParEven,  eulRepYes,  eulFrmS),
        xzy_s = _ORD(eulAxX, eulParOdd,   eulRepNo,   eulFrmS),
        xzx_s = _ORD(eulAxX, eulParOdd,   eulRepYes,  eulFrmS),
        yzx_s = _ORD(eulAxY, eulParEven,  eulRepNo,   eulFrmS),
        yzy_s = _ORD(eulAxY, eulParEven,  eulRepYes,  eulFrmS),
        yxz_s = _ORD(eulAxY, eulParOdd,   eulRepNo,   eulFrmS),
        yxy_s = _ORD(eulAxY, eulParOdd,   eulRepYes,  eulFrmS),
        zxy_s = _ORD(eulAxZ, eulParEven,  eulRepNo,   eulFrmS),
        zxz_s = _ORD(eulAxZ, eulParEven,  eulRepYes,  eulFrmS),
        zyx_s = _ORD(eulAxZ, eulParOdd,   eulRepNo,   eulFrmS),
        zyz_s = _ORD(eulAxZ, eulParOdd,   eulRepYes,  eulFrmS),

        zyx_r = _ORD(eulAxX, eulParEven,  eulRepNo,   eulFrmR),
        xyx_r = _ORD(eulAxX, eulParEven,  eulRepYes,  eulFrmR),
        yzx_r = _ORD(eulAxX, eulParOdd,   eulRepNo,   eulFrmR),
        xzx_r = _ORD(eulAxX, eulParOdd,   eulRepYes,  eulFrmR),
        xzy_r = _ORD(eulAxY, eulParEven,  eulRepNo,   eulFrmR),
        yzy_r = _ORD(eulAxY, eulParEven,  eulRepYes,  eulFrmR),
        zxy_r = _ORD(eulAxY, eulParOdd,   eulRepNo,   eulFrmR),
        yxy_r = _ORD(eulAxY, eulParOdd,   eulRepYes,  eulFrmR),
        yxz_r = _ORD(eulAxZ, eulParEven,  eulRepNo,   eulFrmR),
        zxz_r = _ORD(eulAxZ, eulParEven,  eulRepYes,  eulFrmR),
        xyz_r = _ORD(eulAxZ, eulParOdd,   eulRepNo,   eulFrmR),
        zyz_r = _ORD(eulAxZ, eulParOdd,   eulRepYes,  eulFrmR)
    };
    
    #undef _ORD

    /// Construct with identity
    Quat_()                                                                 : x(0), y(0), z(0), w(1)            {}
    /// Construct with imaginary vector components x,y,z and real scalar component w
    Quat_(Real x, Real y, Real z, Real w)                                   : x(x), y(y), z(z), w(w)            {}
    /// Construct from axis and angle in radians
    Quat_(const Vec3& axis, Real angle)                                     { fromAxisAngle(axis, angle); }
    /// Construct from 3 unit vectors
    Quat_(const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ)          { fromAxes(axisX, axisY, axisZ); }
    /// Construct a quaternion that rotates unit vector from towards unit vector to
    Quat_(const Vec3& from, const Vec3& to)                                 { fromAlign(from, to); }
    /// Construct from euler angles in radians and order `xyz_s`
    explicit Quat_(const Vec3& eulerAngles)                                 { fromEulerAngles(eulerAngles); }
    /// Construct from euler angles in order
    Quat_(const Vec3& eulerAngles, EulerOrder order)                        { fromEulerAngles(eulerAngles, order); }
    /// Construct from 4x4 homogeneous matrix.  Rotation is extracted from upper-left 3x3 submatrix.
    Quat_(const Matrix4& rot)                                               { fromMatrix(rot); }

    /// Set quaternion to zero
    Quat_& fromZero()                                                       { x=0; y=0; z=0; w=0; return *this; }
    /// Set quaternion to identity
    Quat_& fromIdentity()                                                   { x=0; y=0; z=0; w=1; return *this; }
    /// Construct from axis and angle in radians
    Quat_& fromAxisAngle(const Vec3& axis, Real angle);
    /// Construct from euler angles in radians and order `xyz_s`
    Quat_& fromEulerAngles(const Vec3& eulerAngles);
    /// Construct from euler angles in order
    Quat_& fromEulerAngles(const Vec3& eulerAngles, EulerOrder order);
    /// Construct from 4x4 homogeneous matrix.  Rotation is extracted from upper-left 3x3 submatrix.
    Quat_& fromMatrix(const Matrix4& rot);
    /// Construct a quaternion that rotates unit vector v1 towards unit vector v2. The resulting quat's axis is perpendicular to v1 and v2. 
    Quat_& fromAlign(const Vec3& v1, const Vec3& v2);
    /// Construct from 3 unit vectors
    Quat_& fromAxes(const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ);

    bool operator==(const Quat_& rhs) const                                 { return rhs.x == x && rhs.y == y && rhs.z == z && rhs.w == w; }
    bool operator!=(const Quat_& rhs) const                                 { return rhs.x != x || rhs.y != y || rhs.z != z || rhs.w != w; }

    Quat_ operator+() const                                                 { return *this; }
    Quat_ operator+(const Quat_& rhs) const                                 { return Quat_(x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w); }
    Quat_& operator+=(const Quat_& rhs)                                     { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }

    Quat_ operator-() const                                                 { return Quat_(-x, -y, -z, -w); }
    Quat_ operator-(const Quat_& rhs) const                                 { return Quat_(x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w); }
    Quat_& operator-=(const Quat_& rhs)                                     { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }

    Quat_ operator*(const Quat_& rhs) const
    {
        return Quat_(   x*rhs.w + y*rhs.z - z*rhs.y + w*rhs.x,
                        -x*rhs.z + y*rhs.w + z*rhs.x + w*rhs.y,
                        x*rhs.y - y*rhs.x + z*rhs.w + w*rhs.z,
                        -x*rhs.x - y*rhs.y - z*rhs.z + w*rhs.w);
    }

    Vec3 operator*(const Vec3& rhs) const
    {
        Vec3 qvec(x, y, z);
        Vec3 uv = qvec.cross(rhs);
        Vec3 uuv = qvec.cross(uv);
        uv *= w*2;
        uuv *= 2;
        return rhs + uv + uuv;
    }

    Quat_ operator*(Real rhs) const                                         { return Quat_(x*rhs, y*rhs, z*rhs, w*rhs); }
    Quat_& operator*=(const Quat_& rhs)                                     { this->operator=(operator*(rhs)); return *this; }
    Quat_& operator*=(Real rhs)                                             { this->operator=(operator*(rhs)); return *this; }

    Quat_ operator/(const Quat_& rhs) const                                 { return operator*(rhs.inverseNonUnit()); }
    Quat_ operator/(Real rhs) const                                         { return operator*(1/rhs); }
    Quat_& operator/=(const Quat_& rhs)                                     { this->operator=(operator/(rhs)); return *this; }
    Quat_& operator/=(Real rhs)                                             { this->operator=(operator/(rhs)); return *this; }

    friend Quat_ operator*(Real lhs, const Quat_& rhs)                      { return rhs.operator*(lhs); }

    /// Access quaternion components
    const Real& operator[](int i) const                                     { assert(i>=0&&i<dim); return *(&x+i); }
    Real& operator[](int i)                                                 { assert(i>=0&&i<dim); return *(&x+i); }

    /// Cast to array of quaternion components
    operator Real*()                                                        { return &x; }
    operator const Real*() const                                            { return &x; }

    Real dot(const Quat_& q) const                                          { return x*q.x + y*q.y + z*q.z + w*q.w; }
    Quat_ conjugate() const                                                 { return Quat_(-x, -y, -z, w); }
    /// Assumes that quaternion is unit length, same as conjugate()
    Quat_ inverse() const                                                   { return conjugate(); }

    /// Proper quaternion inverse.  Only use if quaternion is expected to be non-unit length.
    Quat_ inverseNonUnit() const
    {
        Real l = lengthSqr();
        if (l > Real_::zeroTol)
        {
            Real l_inv = 1/l;
            return Quat_(-x*l_inv, -y*l_inv, -z*l_inv, w*l_inv);
        }
        return zero;
    }

    Quat_ exp() const;
    Quat_ ln() const;
    Quat_ sqrt() const                                                      { Real tmp = w*2; return Quat_(x*tmp, y*tmp, z*tmp, w*w - x*x - y*y - z*z); }
    /// Square of the length
    Real lengthSqr() const                                                  { return x*x + y*y + z*z + w*w; }
    Real length() const                                                     { return Alge::sqrt(lengthSqr()); }

    /// Get unit quaternion.  The pre-normalized length will be returned in len if specified.
    Quat_ normalize(optional<Real&> len = optnull) const
    {
        Real l = length();
        if (l > Real_::zeroTol)
        {
            if (len) len = l;
            return *this / l;
        }
        if (len) len = 0;
        return zero;
    }

    /// Fast normalization, only accurate when quaternion is close to unit length
    Quat_ normalize_fast() const
    {
        static const Real recurse1 = 0.91521198;
        static const Real recurse2 = 0.65211970;

        Real s = x*x + y*y + z*z + w*w;
        Real k = sqrtInverse_fast(s);
        if (s <= recurse1)
        {
            k = k*sqrtInverse_fast(k*k*s);
            if (s <= recurse2)
                k = k*sqrtInverse_fast(k*k*s);
        }
        return Quat_(x*k, y*k, z*k, w*k);
    }

    /// Get quaternion axis and angle in radians
    void axisAngle(Vec3& axis, Real& angle) const;

    /// Get quaternion's rotated unit axis
    Vec3 axisX() const;
    Vec3 axisY() const;
    Vec3 axisZ() const;

    /// Get unit axes that represent this quaternion
    void axes(Vec3& axisX, Vec3& axisY, Vec3& axisZ) const;

    /// Get euler angles in radians and order `xyz_s`
    Vec3 eulerAngles() const;
    /// Get euler angles in order
    Vec3 eulerAngles(EulerOrder order) const;

    /// Convert quaternion to 4x4 homogeneous rotation matrix.  Set b3x3 to true to store the result only in the upper-left 3x3 submatrix of `rot`, leaving rest of matrix unchanged.
    Matrix4& toMatrix(Matrix4& rot, bool b3x3 = false) const;

    ///Spherical linear interpolation from q0 to q1.  t ranges from [0,1]
    static Quat_ slerp(Real t, const Quat_& q0, const Quat_& q1)
    {
        t = Alge::clamp(t, 0, 1);
        //Make sure we take the short way around the sphere
        Real dot = q0.dot(q1);
        return (dot >= 0) ? slerp_fast(t, q0, q1, dot) : slerp_fast(t, q0, -q1, -dot);
    }

    /// Calc intermediate quats required for Squad. Ex. To interpolate between q1 and q2: setup(q0,q1,q2,q3,a,b,c) -> squad(q1,a,b,c)
    static void squadSetup( const Quat_& q0, const Quat_& q1, const Quat_& q2, const Quat_& q3,
                            Quat_& a, Quat_& b, Quat_& c);

    /// Spherical quadratic interpolation between q1 and c.  t ranges from [0,1].  \see squadSetup()
    static Quat_ squad(Real t, const Quat_& q1, const Quat_& a, const Quat_& b, const Quat_& c);

    /// Triangular bary-centric interpolation
    /**
      * Input -> Output:
      *
      *     (0,0) -> q0              (1,0) -> q1             (0,1) -> q2
      *     1-f-g==0 -> line q1,q2   (f,0) -> line q0,q1     (0,g) -> line q0,q2     
      */ 
    static Quat_ baryCentric(Real f, Real g, const Quat_& q0, const Quat_& q1, const Quat_& q2);

    friend ostream& operator<<(ostream& os, const Quat_& val)
    {
        os << "[";
        for (int i = 0; i < dim; ++i)
        {
            if (i != 0)
                os << ", ";
            os << val[i];
        }
        return os << "]";
    }

private:
    /// Unpacks all information about order
    static void eulGetOrd(EulerOrder ord, int& i, int& j, int& k, int& h, int& n, int& s, int& f)
    {
        static const int eulSafe[] = {0,1,2,0};
        static const int eulNext[] = {1,2,0,1};

        uint32 o = static_cast<uint32>(ord);
        f=o&1; o>>=1;
        s=o&1; o>>=1;
        n=o&1; o>>=1;
        i=eulSafe[o&3]; j=eulNext[i+n]; k=eulNext[i+1-n];
        h=s?k:i;
    }

    /// Fast but less accurate slerp, does not check for shortest path
    static Quat_ slerp_fast(Real t, const Quat_& q0, const Quat_& q1, Real cosAlpha);

    static Real slerpCorrection(Real t, Real cosAlpha)
    {
        static const Real tcor = 0.58549219;
        static const Real tcoratten = 0.82279687;

        Real factor = 1 - tcoratten*cosAlpha;
        factor = factor*factor;
        Real k = tcor*factor;
        Real b = 2 * k;
        Real c = -3 * k;
        Real d = 1 + k;
        return t*(t*(b*t + c) + d);
    }

    /// Fast inverse square root, only accurate when number is close to 1
    static Real sqrtInverse_fast(Real x)
    {
        static const Real neighborhood = 0.959066;
        // static const Real scale = 1.000311;
        static const Real additive = 1.02143;       // scale / sqrt(neighborhood)
        static const Real factor = -0.53235;        // scale * (-0.5 / (neighborhood * sqrt(neighborhood)))
        return additive + (x - neighborhood)*factor;
    }

public:
    Real x;
    Real y;
    Real z;
    Real w;

    static const Quat_ zero;
    static const Quat_ identity;
};

template<class Real> const Quat_<Real> Quat_<Real>::zero        (0, 0, 0, 0);
template<class Real> const Quat_<Real> Quat_<Real>::identity    (0, 0, 0, 1);

typedef Quat_<Real>   Quat;
typedef Quat_<Float>  Quat_f;
typedef Quat_<Double> Quat_d;

extern template class Quat_<Float>;
extern template class Quat_<Double>;
extern template class Quat_<Quad>;

}

#define Section_Header
#include "Honey/Math/Alge/platform/Quat.h"
#undef  Section_Header
