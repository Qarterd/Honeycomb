// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Alge/Quat.h"
#include "Honey/Math/NumAnalysis/Interp.h"

#define Section_Source
#include "Honey/Math/Alge/platform/Quat.h"
#undef  Section_Source

namespace honey
{

template<class Real>
Quat_<Real>& Quat_<Real>::fromAxisAngle(const Vec3& axis, Real angle)
{
    Real halfAngle = angle/2;
    Real sin_a = Trig::sin(halfAngle);
    Real cos_a = Trig::cos(halfAngle);
    x = axis.x*sin_a;
    y = axis.y*sin_a;
    z = axis.z*sin_a;
    w = cos_a;
    return *this;
}

template<class Real>
Quat_<Real>& Quat_<Real>::fromEulerAngles(const Vec3& eulerAngles)
{
    if (eulerAngles.isZero())
    {
        x = 0;
        y = 0;
        z = 0;
        w = 1;
    }
    else
    {
        Real rotx = eulerAngles.x/2;
        Real roty = eulerAngles.y/2;
        Real rotz = eulerAngles.z/2;
        Real sinx = Trig::sin(rotx); Real cosx = Trig::cos(rotx);
        Real siny = Trig::sin(roty); Real cosy = Trig::cos(roty);
        Real sinz = Trig::sin(rotz); Real cosz = Trig::cos(rotz);

        Real cosyz = cosy*cosz;
        Real sinyz = siny*sinz;
        Real sinycosz = siny*cosz;
        Real cosysinz = cosy*sinz;
        x = sinx*cosyz - cosx*sinyz;
        y = cosx*sinycosz + sinx*cosysinz;
        z = cosx*cosysinz - sinx*sinycosz;
        w = cosx*cosyz + sinx*sinyz;
    }

    return *this;
}

template<class Real>
Quat_<Real>& Quat_<Real>::fromEulerAngles(const Vec3& ea_, EulerOrder order)
{
    Vec3 ea = ea_;
    Real a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i,j,k,h,n,s,f;
    eulGetOrd(order,i,j,k,h,n,s,f);
    if (f == eulFrmR) { Real t = ea.x; ea.x = ea.z; ea.z = t; }
    if (n == eulParOdd) ea.y = -ea.y;
    ti = ea.x*0.5; tj = ea.y*0.5; th = ea.z*0.5;
    ci = Trig::cos(ti);  cj = Trig::cos(tj);  ch = Trig::cos(th);
    si = Trig::sin(ti);  sj = Trig::sin(tj);  sh = Trig::sin(th);
    cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
    if (s == eulRepYes)
    {
        a[i] = cj*(cs + sc); // Could speed up with trig identities
        a[j] = sj*(cc + ss);
        a[k] = sj*(cs - sc);
        w = cj*(cc - ss);
    }
    else
    {
        a[i] = cj*sc - sj*cs;
        a[j] = cj*ss + sj*cc;
        a[k] = cj*cs - sj*sc;
        w = cj*cc + sj*ss;
    }
    if (n == eulParOdd) a[j] = -a[j];
    x = a[0]; y = a[1]; z = a[2];
    return *this;
}

template<class Real>
Quat_<Real>& Quat_<Real>::fromMatrix(const Matrix4& rot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    Real trace = rot[0][0]+rot[1][1]+rot[2][2];
    Real root;

    if ( trace > 0 )
    {
        // |w| > 1/2, may as well choose w > 1/2
        root = Alge::sqrt(trace + 1);  // 2w
        w = Real_::half*root;
        root = (root == 0) ? 0 : Real_::half/root;  // 1/(4w)
        x = (rot[2][1]-rot[1][2])*root;
        y = (rot[0][2]-rot[2][0])*root;
        z = (rot[1][0]-rot[0][1])*root;
    }
    else
    {
        // |w| <= 1/2
        static const int s_next[3] = { 1, 2, 0 };
        int i = 0;
        if ( rot[1][1] > rot[0][0] )
            i = 1;
        if ( rot[2][2] > rot[i][i] )
            i = 2;
        int j = s_next[i];
        int k = s_next[j];

        root = Alge::sqrt(rot[i][i]-rot[j][j]-rot[k][k] + 1);
        (*this)[i] = Real_::half*root;
        root = (root == 0) ? 0 : Real_::half/root;
        w = (rot[k][j]-rot[j][k])*root;
        (*this)[j] = (rot[j][i]+rot[i][j])*root;
        (*this)[k] = (rot[k][i]+rot[i][k])*root;
    }

    return *this;
}

template<class Real>
Quat_<Real>& Quat_<Real>::fromAxes(const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ)
{
    Matrix4 rot;
    rot.col(0) = Vec4(axisX);
    rot.col(1) = Vec4(axisY);
    rot.col(2) = Vec4(axisZ);
    return fromMatrix(rot);
}

template<class Real>
Quat_<Real>& Quat_<Real>::fromAlign(const Vec3& v1, const Vec3& v2)
{
    // If V1 and V2 are not parallel, the axis of rotation is the unit-length
    // vector U = cross(V1,V2)/length(cross(V1,V2)).  The angle of rotation,
    // A, is the angle between V1 and V2.  The quaternion for the rotation is
    // q = cos(A/2) + sin(A/2)*(ux*i+uy*j+uz*k) where U = (ux,uy,uz).
    //
    // (1) Rather than extract A = acos(dot(V1,V2)), multiply by 1/2, then
    //     compute sin(A/2) and cos(A/2), we reduce the computational costs by
    //     computing the bisector B = (V1+V2)/length(V1+V2), so cos(A/2) =
    //     dot(V1,B).
    //
    // (2) The rotation axis is U = cross(V1,B)/length(cross(V1,B)), but
    //     length(cross(V1,B)) = length(V1)*length(B)*sin(A/2) = sin(A/2), in
    //     which case sin(A/2)*(ux*i+uy*j+uz*k) = (cx*i+cy*j+cz*k) where
    //     C = cross(V1,B).
    //
    // If V1 = V2, then B = V1, cos(A/2) = 1, and U = (0,0,0).  If V1 = -V2,
    // then B = 0.  This can happen even if V1 is approximately -V2 using
    // floating point arithmetic. In this case, the A = pi and any axis
    // perpendicular to V1 may be used as the rotation axis.

    Vec3 bisector = (v1 + v2).normalize();
    Real cosHalfAngle = v1.dot(bisector);
    w = cosHalfAngle;

    if (!Alge::isNearZero(cosHalfAngle))
    {
        Vec3 cross = v1.cross(bisector);
        x = cross.x;
        y = cross.y;
        z = cross.z;
    }
    else
    {
        //If v1 is zero then there is no rotation
        if (Alge::isNearZero(v1.lengthSqr()))
            return fromIdentity();

        Real invLength;
        if (Alge::abs(v1[0]) >= Alge::abs(v1[1]))
        {
            // V1.x or V1.z is the largest magnitude component.
            invLength = Alge::sqrtInv(v1[0]*v1[0] + v1[2]*v1[2]);
            x = -v1[2]*invLength;
            y = 0;
            z = +v1[0]*invLength;
        }
        else
        {
            // V1.y or V1.z is the largest magnitude component.
            invLength = Alge::sqrtInv(v1[1]*v1[1] + v1[2]*v1[2]);
            x = 0;
            y = +v1[2]*invLength;
            z = -v1[1]*invLength;
        }
    }

    return *this;
}

template<class Real>
Quat_<Real> Quat_<Real>::exp() const
{
    // If q = A*(x*i+y*j+z*k) where (x,y,z) is unit Length, then
    // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k).  If sin(A) is near zero,
    // use exp(q) = cos(A)+A*(x*i+y*j+z*k) since A/sin(A) has limit 1.

    Quat_ res;

    Real angle = Alge::sqrt(x*x + y*y + z*z);

    Real sin = Trig::sin(angle);
    res.w = Trig::cos(angle);

    if (!Alge::isNearZero(sin))
    {
        Real coeff = sin/angle;

        res.x = coeff*x;
        res.y = coeff*y;
        res.z = coeff*z;
    }
    else
    {
        res.x = x;
        res.y = y;
        res.z = z;
    }

    return res;
}

template<class Real>
Quat_<Real> Quat_<Real>::ln() const
{
    // If q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit Length, then
    // ln(q) = A*(x*i+y*j+z*k).  If sin(A) is near zero, use ln(q) =
    // sin(A)*(x*i+y*j+z*k) since sin(A)/A has limit 1.

    Quat_ res;
    res.w = 0;

    if (Alge::abs(w) < 1)
    {
        Real angle = Trig::acos(w);
        Real sin = Trig::sin(angle);
        if (!Alge::isNearZero(sin))
        {
            Real coeff = angle/sin;
            res.x = coeff*x;
            res.y = coeff*y;
            res.z = coeff*z;
            return res;
        }
    }

    res.x = x;
    res.y = y;
    res.z = z;

    return res;
}

template<class Real>
typename Quat_<Real>::Vec3 Quat_<Real>::axisX() const
{
    //Real xd = x*2;
    Real yd = y*2;
    Real zd = z*2;
    Real wy = yd*w;
    Real wz = zd*w;
    Real xy = yd*x;
    Real xz = zd*x;
    Real yy = yd*y;
    Real zz = zd*z;

    return Vec3(1-(yy+zz), xy+wz, xz-wy);
}

template<class Real>
typename Quat_<Real>::Vec3 Quat_<Real>::axisY() const
{
    Real xd  = x*2;
    Real yd  = y*2;
    Real zd  = z*2;
    Real wx = xd*w;
    Real wz = zd*w;
    Real xx = xd*x;
    Real xy = yd*x;
    Real yz = zd*y;
    Real zz = zd*z;

    return Vec3(xy-wz, 1-(xx+zz), yz+wx);
}

template<class Real>
typename Quat_<Real>::Vec3 Quat_<Real>::axisZ() const
{
    Real xd = x*2;
    Real yd = y*2;
    Real zd = z*2;
    Real wx = xd*w;
    Real wy = yd*w;
    Real xx = xd*x;
    Real xz = zd*x;
    Real yy = yd*y;
    Real yz = zd*y;

    return Vec3(xz+wy, yz-wx, 1-(xx+yy));
}

template<class Real>
void Quat_<Real>::axes(Vec3& axisX, Vec3& axisY, Vec3& axisZ) const
{
    Matrix4 rot;
    toMatrix(rot);
    axisX = Vec3(rot.col(0).eval());
    axisY = Vec3(rot.col(1).eval());
    axisZ = Vec3(rot.col(2).eval());
}

template<class Real>
void Quat_<Real>::axisAngle(Vec3& axis, Real& angle) const
{
    // The quaternion representing the rotation is
    //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)

    Real sqrLength = x*x + y*y + z*z;
    if (!Alge::isNearZero(sqrLength))
    {
        Real sqrtLength = Alge::sqrt(sqrLength);
        if (sqrtLength != 0)
        {
            angle = 2 * Trig::acos(w);
            Real invLength = 1 / sqrtLength;
            axis[0] = x*invLength;
            axis[1] = y*invLength;
            axis[2] = z*invLength;
            return;
        }
    }

    // angle is 0 (mod 2*pi), so any axis will do
    angle = 0;
    axis[0] = 1;
    axis[1] = 0;
    axis[2] = 0;
}

template<class Real>
typename Quat_<Real>::Vec3 Quat_<Real>::eulerAngles() const
{
    Vec3 ret;
    Real sqw = w*w;
    Real sqx = x*x;
    Real sqy = y*y;
    Real sqz = z*z;
    Real resy;
    ret.z = Trig::atan2((x*y + z*w)*2, sqx - sqy - sqz + sqw );
    ret.x = Trig::atan2((y*z + x*w)*2, -sqx - sqy + sqz + sqw );
    resy = -((x*z - y*w)*2);
    if (resy <= -1)
        ret.y = -Real_::piHalf;
    else if (resy >= 1)
        ret.y = Real_::piHalf;
    else
        ret.y = Trig::asin(resy);
    
    return ret;
}

template<class Real>
typename Quat_<Real>::Vec3 Quat_<Real>::eulerAngles(EulerOrder order) const
{
    Matrix4 M;
    toMatrix(M, true);

    Vec3 ea;
    int i,j,k,h,n,s,f;
    eulGetOrd(order,i,j,k,h,n,s,f);
    if (s == eulRepYes)
    {
        Real sy = Alge::sqrt(M[i][j]*M[i][j] + M[i][k]*M[i][k]);
        if (!Alge::isNearZero(sy))
        {
            ea.x = Trig::atan2(M[i][j], M[i][k]);
            ea.y = Trig::atan2(sy, M[i][i]);
            ea.z = Trig::atan2(M[j][i], -M[k][i]);
        }
        else
        {
            ea.x = Trig::atan2(-M[j][k], M[j][j]);
            ea.y = Trig::atan2(sy, M[i][i]);
            ea.z = 0;
        }
    }
    else
    {
        Real cy = Alge::sqrt(M[i][i]*M[i][i] + M[j][i]*M[j][i]);
        if (!Alge::isNearZero(cy))
        {
            ea.x = Trig::atan2(M[k][j], M[k][k]);
            ea.y = Trig::atan2(-M[k][i], cy);
            ea.z = Trig::atan2(M[j][i], M[i][i]);
        }
        else
        {
            ea.x = Trig::atan2(-M[j][k], M[j][j]);
            ea.y = Trig::atan2(-M[k][i], cy);
            ea.z = 0;
        }
    }
    if (n==eulParOdd) {ea.x = -ea.x; ea.y = - ea.y; ea.z = -ea.z;}
    if (f==eulFrmR) {Real t = ea.x; ea.x = ea.z; ea.z = t;}
    return ea;
}

template<class Real>
typename Quat_<Real>::Matrix4& Quat_<Real>::toMatrix(Matrix4& rot, bool b3x3) const
{
    Real xd = x*2;
    Real yd = y*2;
    Real zd = z*2;
    Real wx = xd*w;
    Real wy = yd*w;
    Real wz = zd*w;
    Real xx = xd*x;
    Real xy = yd*x;
    Real xz = zd*x;
    Real yy = yd*y;
    Real yz = zd*y;
    Real zz = zd*z;

    rot( 0) = 1-(yy+zz); rot( 1) = xy-wz;     rot( 2) = xz+wy;
    rot( 4) = xy+wz;     rot( 5) = 1-(xx+zz); rot( 6) = yz-wx;
    rot( 8) = xz-wy;     rot( 9) = yz+wx;     rot(10) = 1-(xx+yy);

    if (!b3x3)
    {
        //Fill the 4x4 with identity
        rot( 3) = 0;
        rot( 7) = 0;
        rot(11) = 0;
        rot(12) = 0; rot(13) = 0; rot(14) = 0; rot(15) = 1;
    }

    return rot;
}

template<class Real>
Quat_<Real> Quat_<Real>::slerp_fast(Real t, const Quat_& q0, const Quat_& q1, Real cosAlpha)
{
    //The spline correction diverges after t=0.5, so make sure under 0.5 is passed in
    if (t <= Real_::half)
        t = slerpCorrection(t, cosAlpha);
    else
        t = 1 - slerpCorrection(1 - t, cosAlpha);

    Quat_ ret;
    ret.x = Interp::linear(t, q0.x, q1.x);
    ret.y = Interp::linear(t, q0.y, q1.y);
    ret.z = Interp::linear(t, q0.z, q1.z);
    ret.w = Interp::linear(t, q0.w, q1.w);
    ret = ret.normalize_fast();
    return ret;
}

template<class Real>
void Quat_<Real>::squadSetup(   const Quat_& q0, const Quat_& q1, const Quat_& q2, const Quat_& q3,
                                Quat_& a, Quat_& b, Quat_& c)
{
    Quat_ _q0 = q0.dot(q1) >= 0 ? q0 : -q0;
    c =         q1.dot(q2) >= 0 ? q2 : -q2;
    Quat_ _q3 = q2.dot(q3) >= 0 ? q3 : -q3;

    Quat_ q1inv = q1.conjugate();
    a = q1 * (-(Real_::quarter) * ( (q1inv*_q0).ln() + (q1inv*c).ln() )).exp();

    Quat_ q2inv = c.conjugate();
    b = c * (-(Real_::quarter) * ( (q2inv*q1).ln() + (q2inv*_q3).ln() )).exp();
}

template<class Real>
Quat_<Real> Quat_<Real>::squad(Real t, const Quat_& q1, const Quat_& a, const Quat_& b, const Quat_& c)
{
    t = Alge::clamp(t, 0, 1);
    Quat_ kSlerpQC = slerp_fast(t,q1,c, q1.dot(c));
    Quat_ kSlerpAB = slerp_fast(t,a,b, a.dot(b));
    return slerp_fast(2*t*(1-t), kSlerpQC, kSlerpAB, kSlerpQC.dot(kSlerpAB));
}

template<class Real>
Quat_<Real> Quat_<Real>::baryCentric(Real f, Real g, const Quat_& q0, const Quat_& q1, const Quat_& q2)
{
    f = Alge::clamp(f, 0, 1);
    g = Alge::clamp(g, 0, 1);
    Real t = f+g;
    return (t != 0) ? slerp(g/t, slerp(t, q0, q1), slerp(t, q0, q2)) : q0;
}

template class Quat_<Float>;
template class Quat_<Double>;
template class Quat_<Quad>;

}
