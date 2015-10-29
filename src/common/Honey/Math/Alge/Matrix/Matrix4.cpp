// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Alge/Matrix/Matrix4.h"
#include "Honey/Math/Alge/Transform.h"

#define Section_Source
#include "Honey/Math/Alge/Matrix/platform/Matrix4.h"
#undef Section_Source


namespace honey
{

template<class Real, int O>
const Matrix<4,4,Real,O> Matrix<4,4,Real,O>::zero
(
    0,  0,  0,  0,
    0,  0,  0,  0,
    0,  0,  0,  0,
    0,  0,  0,  0
);

template<class Real, int O>
const Matrix<4,4,Real,O> Matrix<4,4,Real,O>::identity
(
    1,  0,  0,  0,
    0,  1,  0,  0,
    0,  0,  1,  0,
    0,  0,  0,  1
);

template<class Real, int O>
Matrix<4,4,Real,O>& Matrix<4,4,Real,O>::fromTm(const Transform& tm)
{
    return fromTrs(tm.getTrans(), tm.getRot(), tm.getScale(), tm.getSkew());
}

template<class Real, int O>
Matrix<4,4,Real,O>& Matrix<4,4,Real,O>::fromObliqueProjection(const Vec3& normal, const Vec3& point, const Vec3& dir)
{
    // The projection plane is dot(N,X-P) = 0 where N is a 3-by-1 unit-length
    // normal vector and P is a 3-by-1 poInt on the plane.  The projection
    // is oblique to the plane, in the direction of the 3-by-1 vector D.
    // Necessarily dot(N,D) is not zero for this projection to make sense.
    // Given a 3-by-1 poInt U, compute the intersection of the line U+t*D
    // with the plane to obtain t = -dot(N,U-P)/dot(N,D).  Then
    //
    //   projection(U) = P + [I - D*N^T/dot(N,D)]*(U-P)
    //
    // A 4-by-4 homogeneous transformation representing the projection is
    //
    //       +-                               -+
    //   M = | D*N^T - dot(N,D)*I   -dot(N,P)D |
    //       |          0^T          -dot(N,D) |
    //       +-                               -+
    //
    // where M applies to [U^T 1]^T by M*[U^T 1]^T.  The matrix is chosen so
    // that M[3][3] > 0 whenever dot(N,D) < 0 (projection is onto the
    // "positive side" of the plane).

    Real ndD = normal.dot(dir);
    Real ndP = normal.dot(point);
    m( 0) = dir[0]*normal[0] - ndD;
    m( 1) = dir[0]*normal[1];
    m( 2) = dir[0]*normal[2];
    m( 3) = -ndP*dir[0];
    m( 4) = dir[1]*normal[0];
    m( 5) = dir[1]*normal[1] - ndD;
    m( 6) = dir[1]*normal[2];
    m( 7) = -ndP*dir[1];
    m( 8) = dir[2]*normal[0];
    m( 9) = dir[2]*normal[1];
    m(10) = dir[2]*normal[2] - ndD;
    m(11) = -ndP*dir[2];
    m(12) = 0;
    m(13) = 0;
    m(14) = 0;
    m(15) = -ndD;

    return *this;
}

template<class Real, int O>
Matrix<4,4,Real,O>& Matrix<4,4,Real,O>::fromPerspectiveProjection(const Vec3& normal, const Vec3& point, const Vec3& eye)
{
    //     +-                                                 -+
    // M = | dot(N,E-P)*I - E*N^T    -(dot(N,E-P)*I - E*N^T)*E |
    //     |        -N^t                      dot(N,E)         |
    //     +-                                                 -+
    //
    // where E is the eye point, P is a poInt on the plane, and N is a
    // unit-length plane normal.

    Real ndEmP = normal.dot(eye-point);

    m( 0) = ndEmP - eye[0]*normal[0];
    m( 1) = -eye[0]*normal[1];
    m( 2) = -eye[0]*normal[2];
    m( 3) = -(m(0)*eye[0] + m(1)*eye[1] + m(2)*eye[2]);
    m( 4) = -eye[1]*normal[0];
    m( 5) = ndEmP - eye[1]*normal[1];
    m( 6) = -eye[1]*normal[2];
    m( 7) = -(m(4)*eye[0] + m(5)*eye[1] + m(6)*eye[2]);
    m( 8) = -eye[2]*normal[0];
    m( 9) = -eye[2]*normal[1];
    m(10) = ndEmP- eye[2]*normal[2];
    m(11) = -(m(8)*eye[0] + m(9)*eye[1] + m(10)*eye[2]);
    m(12) = -normal[0];
    m(13) = -normal[1];
    m(14) = -normal[2];
    m(15) = normal.dot(eye);

    return *this;
}

template<class Real, int O>
Matrix<4,4,Real,O>& Matrix<4,4,Real,O>::fromReflection(const Vec3& normal, const Vec3& point)
{
    //     +-                         -+
    // M = | I-2*N*N^T    2*dot(N,P)*N |
    //     |     0^T            1      |
    //     +-                         -+
    //
    // where P is a poInt on the plane and N is a unit-length plane normal.

    Real twoNdP = 2*(normal.dot(point));

    m( 0) = 1 - 2*normal[0]*normal[0];
    m( 1) = -2*normal[0]*normal[1];
    m( 2) = -2*normal[0]*normal[2];
    m( 3) = twoNdP*normal[0];
    m( 4) = -2*normal[1]*normal[0];
    m( 5) = 1 - 2*normal[1]*normal[1];
    m( 6) = -2*normal[1]*normal[2];
    m( 7) = twoNdP*normal[1];
    m( 8) = -2*normal[2]*normal[0];
    m( 9) = -2*normal[2]*normal[1];
    m(10) = 1 - 2*normal[2]*normal[2];
    m(11) = twoNdP*normal[2];
    m(12) = 0;
    m(13) = 0;
    m(14) = 0;
    m(15) = 1;

    return *this;
}

template<class Real, int O>
Matrix<4,4,Real,O>& Matrix<4,4,Real,O>::fromLookAt(const Vec3& eye, const Vec3& at, const Vec3& up)
{
    Vec3 z = (eye - at).normalize();
    Vec3 x = up.crossUnit(z);
    Vec3 y = z.cross(x);
    this->row(0) = Vec4(x, -x.dot(eye));
    this->row(1) = Vec4(y, -y.dot(eye));
    this->row(2) = Vec4(z, -z.dot(eye));
    this->row(3) = Vec4::axisW;
    return *this;
}

template<class Real, int O>
void Matrix<4,4,Real,O>::orthonormalize()
{
    // Algorithm uses Gram-Schmidt orthogonalization.  If 'this' matrix has
    // upper-left 3x3 block M = [m0|m1|m2], then the orthonormal output matrix
    // is Q = [q0|q1|q2],
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.

    // Compute q0.
    Real invLength = 1 / Alge::sqrt(m(0)*m(0) + m(4)*m(4) + m(8)*m(8));

    m(0) *= invLength;
    m(4) *= invLength;
    m(8) *= invLength;

    // Compute q1.
    Real dot0 = m(0)*m(1) + m(4)*m(5) + m(8)*m(9);

    m(1) -= dot0*m(0);
    m(5) -= dot0*m(4);
    m(9) -= dot0*m(8);

    invLength = 1 / Alge::sqrt(m(1)*m(1) + m(5)*m(5) + m(9)*m(9));

    m(1) *= invLength;
    m(5) *= invLength;
    m(9) *= invLength;

    // Compute q2.
    Real dot1 = m(1)*m(2) + m(5)*m(6) + m(9)*m(10);

    dot0 = m(0)*m(2) + m(4)*m(6) + m(8)*m(10);

    m( 2) -= dot0*m(0) + dot1*m(1);
    m( 6) -= dot0*m(4) + dot1*m(5);
    m(10) -= dot0*m(8) + dot1*m(9);

    invLength = 1 / Alge::sqrt(m(2)*m(2) + m(6)*m(6) + m(10)*m(10));

    m( 2) *= invLength;
    m( 6) *= invLength;
    m(10) *= invLength;
}

template<class Real, int O>
Matrix<4,4,Real,O> Matrix<4,4,Real,O>::inverse(optional<Real&> det) const
{
    Real a0 = m( 0)*m( 5) - m( 1)*m( 4);
    Real a1 = m( 0)*m( 6) - m( 2)*m( 4);
    Real a2 = m( 0)*m( 7) - m( 3)*m( 4);
    Real a3 = m( 1)*m( 6) - m( 2)*m( 5);
    Real a4 = m( 1)*m( 7) - m( 3)*m( 5);
    Real a5 = m( 2)*m( 7) - m( 3)*m( 6);
    Real b0 = m( 8)*m(13) - m( 9)*m(12);
    Real b1 = m( 8)*m(14) - m(10)*m(12);
    Real b2 = m( 8)*m(15) - m(11)*m(12);
    Real b3 = m( 9)*m(14) - m(10)*m(13);
    Real b4 = m( 9)*m(15) - m(11)*m(13);
    Real b5 = m(10)*m(15) - m(11)*m(14);

    Real d = a0*b5-a1*b4+a2*b3+a3*b2-a4*b1+a5*b0;
    if (det) det = d;
    if (Alge::isNearZero(d))
    {
        if (det) det = 0;
        return zero;
    }

    Matrix inv;
    inv( 0) = + m( 5)*b5 - m( 6)*b4 + m( 7)*b3;
    inv( 4) = - m( 4)*b5 + m( 6)*b2 - m( 7)*b1;
    inv( 8) = + m( 4)*b4 - m( 5)*b2 + m( 7)*b0;
    inv(12) = - m( 4)*b3 + m( 5)*b1 - m( 6)*b0;
    inv( 1) = - m( 1)*b5 + m( 2)*b4 - m( 3)*b3;
    inv( 5) = + m( 0)*b5 - m( 2)*b2 + m( 3)*b1;
    inv( 9) = - m( 0)*b4 + m( 1)*b2 - m( 3)*b0;
    inv(13) = + m( 0)*b3 - m( 1)*b1 + m( 2)*b0;
    inv( 2) = + m(13)*a5 - m(14)*a4 + m(15)*a3;
    inv( 6) = - m(12)*a5 + m(14)*a2 - m(15)*a1;
    inv(10) = + m(12)*a4 - m(13)*a2 + m(15)*a0;
    inv(14) = - m(12)*a3 + m(13)*a1 - m(14)*a0;
    inv( 3) = - m( 9)*a5 + m(10)*a4 - m(11)*a3;
    inv( 7) = + m( 8)*a5 - m(10)*a2 + m(11)*a1;
    inv(11) = - m( 8)*a4 + m( 9)*a2 - m(11)*a0;
    inv(15) = + m( 8)*a3 - m( 9)*a1 + m(10)*a0;
    inv /= d;
    return inv;
}

template<class Real, int O>
Matrix<4,4,Real,O> Matrix<4,4,Real,O>::adjugate() const
{
    Real a0 = m( 0)*m( 5) - m( 1)*m( 4);
    Real a1 = m( 0)*m( 6) - m( 2)*m( 4);
    Real a2 = m( 0)*m( 7) - m( 3)*m( 4);
    Real a3 = m( 1)*m( 6) - m( 2)*m( 5);
    Real a4 = m( 1)*m( 7) - m( 3)*m( 5);
    Real a5 = m( 2)*m( 7) - m( 3)*m( 6);
    Real b0 = m( 8)*m(13) - m( 9)*m(12);
    Real b1 = m( 8)*m(14) - m(10)*m(12);
    Real b2 = m( 8)*m(15) - m(11)*m(12);
    Real b3 = m( 9)*m(14) - m(10)*m(13);
    Real b4 = m( 9)*m(15) - m(11)*m(13);
    Real b5 = m(10)*m(15) - m(11)*m(14);

    Matrix adj;
    adj( 0) = + m( 5)*b5 - m( 6)*b4 + m( 7)*b3;
    adj( 4) = - m( 4)*b5 + m( 6)*b2 - m( 7)*b1;
    adj( 8) = + m( 4)*b4 - m( 5)*b2 + m( 7)*b0;
    adj(12) = - m( 4)*b3 + m( 5)*b1 - m( 6)*b0;
    adj( 1) = - m( 1)*b5 + m( 2)*b4 - m( 3)*b3;
    adj( 5) = + m( 0)*b5 - m( 2)*b2 + m( 3)*b1;
    adj( 9) = - m( 0)*b4 + m( 1)*b2 - m( 3)*b0;
    adj(13) = + m( 0)*b3 - m( 1)*b1 + m( 2)*b0;
    adj( 2) = + m(13)*a5 - m(14)*a4 + m(15)*a3;
    adj( 6) = - m(12)*a5 + m(14)*a2 - m(15)*a1;
    adj(10) = + m(12)*a4 - m(13)*a2 + m(15)*a0;
    adj(14) = - m(12)*a3 + m(13)*a1 - m(14)*a0;
    adj( 3) = - m( 9)*a5 + m(10)*a4 - m(11)*a3;
    adj( 7) = + m( 8)*a5 - m(10)*a2 + m(11)*a1;
    adj(11) = - m( 8)*a4 + m( 9)*a2 - m(11)*a0;
    adj(15) = + m( 8)*a3 - m( 9)*a1 + m(10)*a0;

    return adj;
}

template<class Real, int O>
auto Matrix<4,4,Real,O>::determinant() const -> Real
{
    Real a0 = m( 0)*m( 5) - m( 1)*m( 4);
    Real a1 = m( 0)*m( 6) - m( 2)*m( 4);
    Real a2 = m( 0)*m( 7) - m( 3)*m( 4);
    Real a3 = m( 1)*m( 6) - m( 2)*m( 5);
    Real a4 = m( 1)*m( 7) - m( 3)*m( 5);
    Real a5 = m( 2)*m( 7) - m( 3)*m( 6);
    Real b0 = m( 8)*m(13) - m( 9)*m(12);
    Real b1 = m( 8)*m(14) - m(10)*m(12);
    Real b2 = m( 8)*m(15) - m(11)*m(12);
    Real b3 = m( 9)*m(14) - m(10)*m(13);
    Real b4 = m( 9)*m(15) - m(11)*m(13);
    Real b5 = m(10)*m(15) - m(11)*m(14);
    Real det = a0*b5-a1*b4+a2*b3+a3*b2-a4*b1+a5*b0;
    return det;
}

template<class Real, int O>
void Matrix<4,4,Real,O>::decompose( optional<Vec3&> trans, optional<Quat&> rot,
                                    optional<Vec3&> scale, optional<Quat&> skew) const
{
    //Get scale and shear.
    Vec3 row[3], scl;

    row[0] = Vec3(this->col(0).eval());
    row[1] = Vec3(this->col(1).eval());
    row[2] = Vec3(this->col(2).eval());

    // Compute X scale factor and normalize first row.
    row[0] = row[0].normalize(scl.x);

    // Compute XY shear factor and make 2nd row orthogonal to 1st.
    row[1] = row[1] - row[0] * row[0].dot(row[1]);

    // Compute Y scale and normalize 2nd row.
    row[1] = row[1].normalize(scl.y);

    // Compute XZ and YZ shears, orthogonalize 3rd row.
    row[2] = row[2] - row[0] * row[0].dot(row[2]);
    row[2] = row[2] - row[1] * row[1].dot(row[2]);

    // Get Z scale and normalize 3rd row.
    row[2] = row[2].normalize(scl.z);

    if (skew)
    {
        //Skew algorithm is expensive, only do it if scale is non-uniform.
        Real tol = scl.x * 1.0e-4;
        if (!Alge::isNear(scl.x, scl.y, tol) || !Alge::isNear(scl.x, scl.z, tol))
        {
            decomposeSkew(trans, rot, scale, skew);
            return;
        }
        skew = Quat::identity;
    }

    // At this point, the matrix (in rows[]) is orthonormal.
    // Check for a coordinate system flip.  If the determinant
    // is -1, then negate the matrix and the scaling factors.
    
    if (row[0].dot(row[1].cross(row[2])) < 0)
    {
        row[0] = -row[0];
        row[1] = -row[1];
        row[2] = -row[2];
        scl.x = -scl.x;
        scl.y = -scl.y;
        scl.z = -scl.z;
    }

    if (trans) trans = getTrans();
    if (scale) scale = scl;

    // Get the rotations out
    if (rot)
    {
        Vec3 vrot;
        vrot.y = Trig::asin(-row[0].z);
        if (Trig::cos(vrot.y) != 0)
        {
            vrot.x = Trig::atan2(row[1].z, row[2].z);
            vrot.z = Trig::atan2(row[0].y, row[0].x);
        }
        else
        {
            vrot.x = Trig::atan2(row[1].x, row[1].y);
            vrot.z = 0;
        }   
        rot->fromEulerAngles(vrot);
    }
}

//==============================================================================================================
//  Algorithm to handle special case where skew is required in decomposition
//==============================================================================================================
/**** Decompose.h ****/
/* Ken Shoemake, 1993 */
template<class Real>
class DecompAffine
{
    typedef Alge_<Real> Alge;

public:
    typedef struct {Real x, y, z, w;} Quat; /* Quat */
    enum QuatPart {X, Y, Z, W};
    typedef Quat HVect; /* Homogeneous 3D vector */
    typedef Real HMatrix[4][4]; /* Right-handed, for column vectors */

    typedef struct {
        HVect t;    /* Translation components */
        Quat  q;    /* Essential rotation      */
        Quat  u;    /* Stretch rotation      */
        HVect k;    /* Stretch factors      */
        Real f;     /* Sign of determinant      */
    } AffineParts;

    static Real polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
    static HVect spect_decomp(HMatrix S, HMatrix U);
    static Quat snuggle(Quat q, HVect *k);
    static void decomp_affine(HMatrix A, AffineParts *parts);
    static void invert_affine(AffineParts *parts, AffineParts *inverse);

private:
    static void mat_mult(HMatrix A, HMatrix B, HMatrix AB);
    static Real vdot(Real *va, Real *vb);
    static void vcross(Real *va, Real *vb, Real *v);
    static void adjoint_transpose(HMatrix M, HMatrix MadjT);
    static Quat Qt_(Real x, Real y, Real z, Real w);
    static Quat Qt_Conj(Quat q);
    static Quat Qt_Mul(Quat l, Quat r);
    static Quat Qt_Scale(Quat q, Real w);
    static Quat Qt_FromMatrix(HMatrix mat);
    static HMatrix mat_id;
    static Real mat_norm(HMatrix M, int tpose);
    static Real norm_inf(HMatrix M);
    static Real norm_one(HMatrix M);
    static int find_max_col(HMatrix M);
    static void make_reflector(Real *v, Real *u);
    static void reflect_cols(HMatrix M, Real *u);
    static void reflect_rows(HMatrix M, Real *u);
    static void do_rank1(HMatrix M, HMatrix Q);
    static void do_rank2(HMatrix M, HMatrix MadjT, HMatrix Q);
};

/**** Decompose.c ****/
/* Ken Shoemake, 1993 */

/******* Matrix Preliminaries *******/

/** Fill out 3x3 matrix to 4x4 **/
#define mat_pad(A) (A[W][X]=A[X][W]=A[W][Y]=A[Y][W]=A[W][Z]=A[Z][W]=0,A[W][W]=1)

/** Copy nxn matrix A to C using "gets" for assignment **/
#define mat_copy(C,gets,A,n) {int i,j; for(i=0;i<n;i++) for(j=0;j<n;j++)\
    C[i][j] gets (A[i][j]);}

/** Copy transpose of nxn matrix A to C using "gets" for assignment **/
#define mat_tpose(AT,gets,A,n) {int i,j; for(i=0;i<n;i++) for(j=0;j<n;j++)\
    AT[i][j] gets (A[j][i]);}

/** Assign nxn matrix C the element-wise combination of A and B using "op" **/
#define mat_binop(C,gets,A,op,B,n) {int i,j; for(i=0;i<n;i++) for(j=0;j<n;j++)\
    C[i][j] gets (A[i][j]) op (B[i][j]);}

/** Multiply the upper left 3x3 parts of A and B to get AB **/
template<class Real>
void DecompAffine<Real>::mat_mult(HMatrix A, HMatrix B, HMatrix AB)
{
    int i, j;
    for (i=0; i<3; i++) for (j=0; j<3; j++)
    AB[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + A[i][2]*B[2][j];
}

/** Return dot product of length 3 vectors va and vb **/
template<class Real>
Real DecompAffine<Real>::vdot(Real *va, Real *vb)
{
    return (va[0]*vb[0] + va[1]*vb[1] + va[2]*vb[2]);
}

/** Set v to cross product of length 3 vectors va and vb **/
template<class Real>
void DecompAffine<Real>::vcross(Real *va, Real *vb, Real *v)
{
    v[0] = va[1]*vb[2] - va[2]*vb[1];
    v[1] = va[2]*vb[0] - va[0]*vb[2];
    v[2] = va[0]*vb[1] - va[1]*vb[0];
}

/** Set MadjT to transpose of inverse of M times determinant of M **/
template<class Real>
void DecompAffine<Real>::adjoint_transpose(HMatrix M, HMatrix MadjT)
{
    vcross(M[1], M[2], MadjT[0]);
    vcross(M[2], M[0], MadjT[1]);
    vcross(M[0], M[1], MadjT[2]);
}

/******* Quat Preliminaries *******/

/* Construct a (possibly non-unit) quaternion from real components. */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::Qt_(Real x, Real y, Real z, Real w)
{
    Quat qq;
    qq.x = x; qq.y = y; qq.z = z; qq.w = w;
    return (qq);
}

/* Return conjugate of quaternion. */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::Qt_Conj(Quat q)
{
    Quat qq;
    qq.x = -q.x; qq.y = -q.y; qq.z = -q.z; qq.w = q.w;
    return (qq);
}

/** Return quaternion product l * r.  Note: order is important!
  * To combine rotations, use the product mul(second, first),
  * which gives the effect of rotating by first then second. */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::Qt_Mul(Quat l, Quat r)
{
    Quat qq;
    qq.w = l.w*r.w - l.x*r.x - l.y*r.y - l.z*r.z;
    qq.x = l.w*r.x + l.x*r.w + l.y*r.z - l.z*r.y;
    qq.y = l.w*r.y + l.y*r.w + l.z*r.x - l.x*r.z;
    qq.z = l.w*r.z + l.z*r.w + l.x*r.y - l.y*r.x;
    return (qq);
}

/* Return product of quaternion q by scalar w. */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::Qt_Scale(Quat q, Real w)
{
    Quat qq;
    qq.w = q.w*w; qq.x = q.x*w; qq.y = q.y*w; qq.z = q.z*w;
    return (qq);
}

/* Construct a unit quaternion from rotation matrix.  Assumes matrix is
  * used to multiply column vector on the left: vnew = mat vold.     Works
  * correctly for right-handed coordinate system and right-handed rotations.
  * Translation and perspective components ignored. */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::Qt_FromMatrix(HMatrix mat)
{
    /* This algorithm avoids near-zero divides by looking for a large component
      * - first w, then x, y, or z.  When the trace is greater than zero,
      *|w| is greater than 1/2, which is as small as a largest component can be.
      * Otherwise, the largest diagonal entry corresponds to the largest of |x|,
      *|y|, or |z|, one of which must be larger than |w|, and at least 1/2. */
    Quat qu; qu.x = 0;
    Real tr, s;

    tr = mat[X][X] + mat[Y][Y]+ mat[Z][Z];
    if (tr >= 0.0) {
        s = Alge::sqrt(tr + mat[W][W]);
        qu.w = s*0.5;
        s = 0.5 / s;
        qu.x = (mat[Z][Y] - mat[Y][Z]) * s;
        qu.y = (mat[X][Z] - mat[Z][X]) * s;
        qu.z = (mat[Y][X] - mat[X][Y]) * s;
    } else {
        int h = X;
        if (mat[Y][Y] > mat[X][X]) h = Y;
        if (mat[Z][Z] > mat[h][h]) h = Z;
        switch (h) {
#define caseMacro(i,j,k,I,J,K) \
        case I:\
        s = Alge::sqrt( (mat[I][I] - (mat[J][J]+mat[K][K])) + mat[W][W] );\
        qu.i = s*0.5;\
        s = 0.5 / s;\
        qu.j = (mat[I][J] + mat[J][I]) * s;\
        qu.k = (mat[K][I] + mat[I][K]) * s;\
        qu.w = (mat[K][J] - mat[J][K]) * s;\
        break
        caseMacro(x,y,z,X,Y,Z);
        caseMacro(y,z,x,Y,Z,X);
        caseMacro(z,x,y,Z,X,Y);
        }
    }
    if (mat[W][W] != 1.0) qu = Qt_Scale(qu, 1/Alge::sqrt(mat[W][W]));
    return (qu);
}
/******* Decomp Auxiliaries *******/

template<class Real>
typename DecompAffine<Real>::HMatrix DecompAffine<Real>::mat_id = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};

/** Compute either the 1 or infinity norm of M, depending on tpose **/
template<class Real>
Real DecompAffine<Real>::mat_norm(HMatrix M, int tpose)
{
    int i;
    Real sum, max;
    max = 0.0;
    for (i=0; i<3; i++) {
    if (tpose) sum = Alge::abs(M[0][i])+Alge::abs(M[1][i])+Alge::abs(M[2][i]);
    else       sum = Alge::abs(M[i][0])+Alge::abs(M[i][1])+Alge::abs(M[i][2]);
    if (max<sum) max = sum;
    }
    return max;
}

template<class Real>
Real DecompAffine<Real>::norm_inf(HMatrix M) {return mat_norm(M, 0);}
template<class Real>
Real DecompAffine<Real>::norm_one(HMatrix M) {return mat_norm(M, 1);}

/** Return index of column of M containing maximum abs entry, or -1 if M=0 **/
template<class Real>
int DecompAffine<Real>::find_max_col(HMatrix M)
{
    Real abs, max;
    int i, j, col;
    max = 0.0; col = -1;
    for (i=0; i<3; i++) for (j=0; j<3; j++) {
    abs = M[i][j]; if (abs<0.0) abs = -abs;
    if (abs>max) {max = abs; col = j;}
    }
    return col;
}

/** Set up u for Household reflection to zero all v components but first **/
template<class Real>
void DecompAffine<Real>::make_reflector(Real *v, Real *u)
{
    Real s = Alge::sqrt(vdot(v, v));
    u[0] = v[0]; u[1] = v[1];
    u[2] = v[2] + ((v[2]<0.0) ? -s : s);
    s = Alge::sqrt(2.0/vdot(u, u));
    u[0] = u[0]*s; u[1] = u[1]*s; u[2] = u[2]*s;
}

/** Apply Householder reflection represented by u to column vectors of M **/
template<class Real>
void DecompAffine<Real>::reflect_cols(HMatrix M, Real *u)
{
    int i, j;
    for (i=0; i<3; i++) {
    Real s = u[0]*M[0][i] + u[1]*M[1][i] + u[2]*M[2][i];
    for (j=0; j<3; j++) M[j][i] -= u[j]*s;
    }
}
/** Apply Householder reflection represented by u to row vectors of M **/
template<class Real>
void DecompAffine<Real>::reflect_rows(HMatrix M, Real *u)
{
    int i, j;
    for (i=0; i<3; i++) {
    Real s = vdot(u, M[i]);
    for (j=0; j<3; j++) M[i][j] -= u[j]*s;
    }
}

/** Find orthogonal factor Q of rank 1 (or less) M **/
template<class Real>
void DecompAffine<Real>::do_rank1(HMatrix M, HMatrix Q)
{
    Real v1[3], v2[3], s;
    int col;
    mat_copy(Q,=,mat_id,4);
    /* If rank(M) is 1, we should find a non-zero column in M */
    col = find_max_col(M);
    if (col<0) return; /* Rank is 0 */
    v1[0] = M[0][col]; v1[1] = M[1][col]; v1[2] = M[2][col];
    make_reflector(v1, v1); reflect_cols(M, v1);
    v2[0] = M[2][0]; v2[1] = M[2][1]; v2[2] = M[2][2];
    make_reflector(v2, v2); reflect_rows(M, v2);
    s = M[2][2];
    if (s<0.0) Q[2][2] = -1.0;
    reflect_cols(Q, v1); reflect_rows(Q, v2);
}

/** Find orthogonal factor Q of rank 2 (or less) M using adjoInt transpose **/
template<class Real>
void DecompAffine<Real>::do_rank2(HMatrix M, HMatrix MadjT, HMatrix Q)
{
    Real v1[3], v2[3];
    Real w, x, y, z, c, s, d;
    int col;
    /* If rank(M) is 2, we should find a non-zero column in MadjT */
    col = find_max_col(MadjT);
    if (col<0) {do_rank1(M, Q); return;} /* Rank<2 */
    v1[0] = MadjT[0][col]; v1[1] = MadjT[1][col]; v1[2] = MadjT[2][col];
    make_reflector(v1, v1); reflect_cols(M, v1);
    vcross(M[0], M[1], v2);
    make_reflector(v2, v2); reflect_rows(M, v2);
    w = M[0][0]; x = M[0][1]; y = M[1][0]; z = M[1][1];
    if (w*z>x*y) {
    c = z+w; s = y-x; d = Alge::sqrt(c*c+s*s); c = c/d; s = s/d;
    Q[0][0] = Q[1][1] = c; Q[0][1] = -(Q[1][0] = s);
    } else {
    c = z-w; s = y+x; d = Alge::sqrt(c*c+s*s); c = c/d; s = s/d;
    Q[0][0] = -(Q[1][1] = c); Q[0][1] = Q[1][0] = s;
    }
    Q[0][2] = Q[2][0] = Q[1][2] = Q[2][1] = 0.0; Q[2][2] = 1.0;
    reflect_cols(Q, v1); reflect_rows(Q, v2);
}


/******* Polar Decomposition *******/

/** Polar Decomposition of 3x3 matrix in 4x4,
  * M = QS.  See Nicholas Higham and Robert S. Schreiber,
  * Fast Polar Decomposition of An Arbitrary Matrix,
  * Technical Report 88-942, October 1988,
  * Department of Computer Science, Cornell University.
  */
template<class Real>
Real DecompAffine<Real>::polar_decomp(HMatrix M, HMatrix Q, HMatrix S)
{
#define TOL 1.0e-6
    HMatrix Mk, MadjTk, Ek;
    Real det, M_one, M_inf, MadjT_one, MadjT_inf, E_one, gamma, g1, g2;
    int i, j;
    mat_tpose(Mk,=,M,3);
    M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
    do {
    adjoint_transpose(Mk, MadjTk);
    det = vdot(Mk[0], MadjTk[0]);
    if (det==0.0) {do_rank2(Mk, MadjTk, Mk); break;}
    MadjT_one = norm_one(MadjTk); MadjT_inf = norm_inf(MadjTk);
    gamma = Alge::sqrt(Alge::sqrt((MadjT_one*MadjT_inf)/(M_one*M_inf))/Alge::abs(det));
    g1 = gamma*0.5;
    g2 = 0.5/(gamma*det);
    mat_copy(Ek,=,Mk,3);
    mat_binop(Mk,=,g1*Mk,+,g2*MadjTk,3);
    mat_copy(Ek,-=,Mk,3);
    E_one = norm_one(Ek);
    M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
    } while (E_one>(M_one*TOL));
    mat_tpose(Q,=,Mk,3); mat_pad(Q);
    mat_mult(Mk, M, S);     mat_pad(S);
    for (i=0; i<3; i++) for (j=i; j<3; j++)
    S[i][j] = S[j][i] = 0.5*(S[i][j]+S[j][i]);
    return (det);
}


/******* Spectral Decomposition *******/

/** Compute the spectral decomposition of symmetric positive semi-definite S.
  * Returns rotation in U and scale factors in result, so that if K is a diagonal
  * matrix of the scale factors, then S = U K (U transpose). Uses Jacobi method.
  * See Gene H. Golub and Charles F. Van Loan. Matrix Computations. Hopkins 1983.
  */
template<class Real>
typename DecompAffine<Real>::HVect DecompAffine<Real>::spect_decomp(HMatrix S, HMatrix U)
{
    HVect kv;
    Real Diag[3],OffD[3]; /* OffD is off-diag (by omitted index) */
    Real g,h,fabsh,fabsOffDi,t,theta,c,s,tau,ta,OffDq,a,b;
    static char nxt[] = {Y,Z,X};
    int sweep, i, j;
    mat_copy(U,=,mat_id,4);
    Diag[X] = S[X][X]; Diag[Y] = S[Y][Y]; Diag[Z] = S[Z][Z];
    OffD[X] = S[Y][Z]; OffD[Y] = S[Z][X]; OffD[Z] = S[X][Y];
    for (sweep=20; sweep>0; sweep--) {
    Real sm = Alge::abs(OffD[X])+Alge::abs(OffD[Y])+Alge::abs(OffD[Z]);
    if (sm==0.0) break;
    for (i=Z; i>=X; i--) {
        int p = nxt[i]; int q = nxt[p];
        fabsOffDi = Alge::abs(OffD[i]);
        g = 100.0*fabsOffDi;
        if (fabsOffDi>0.0) {
        h = Diag[q] - Diag[p];
        fabsh = Alge::abs(h);
        if (fabsh+g==fabsh) {
            t = OffD[i]/h;
        } else {
            theta = 0.5*h/OffD[i];
            t = 1.0/(Alge::abs(theta)+Alge::sqrt(theta*theta+1.0));
            if (theta<0.0) t = -t;
        }
        c = 1.0/Alge::sqrt(t*t+1.0); s = t*c;
        tau = s/(c+1.0);
        ta = t*OffD[i]; OffD[i] = 0.0;
        Diag[p] -= ta; Diag[q] += ta;
        OffDq = OffD[q];
        OffD[q] -= s*(OffD[p] + tau*OffD[q]);
        OffD[p] += s*(OffDq   - tau*OffD[p]);
        for (j=Z; j>=X; j--) {
            a = U[j][p]; b = U[j][q];
            U[j][p] -= s*(b + tau*a);
            U[j][q] += s*(a - tau*b);
        }
        }
    }
    }
    kv.x = Diag[X]; kv.y = Diag[Y]; kv.z = Diag[Z]; kv.w = 1.0;
    return (kv);
}

/******* Spectral Axis Adjustment *******/

/** Given a unit quaternion, q, and a scale vector, k, find a unit quaternion, p,
  * which permutes the axes and turns freely in the plane of duplicate scale
  * factors, such that q p has the largest possible w component, i.e. the
  * smallest possible angle. Permutes k's components to go with q p instead of q.
  * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
  * Proceedings of Graphics Interface 1992. Details on p. 262-263.
  */
template<class Real>
typename DecompAffine<Real>::Quat DecompAffine<Real>::snuggle(Quat q, HVect *k)
{
#define SQRTHALF (0.7071067811865475244)
#define sgn(n,v)    ((n)?-(v):(v))
#define swap(a,i,j) {a[3]=a[i]; a[i]=a[j]; a[j]=a[3];}
#define cycle(a,p)  if (p) {a[3]=a[0]; a[0]=a[1]; a[1]=a[2]; a[2]=a[3];}\
            else   {a[3]=a[2]; a[2]=a[1]; a[1]=a[0]; a[0]=a[3];}
    Quat p; p.x = 0;
    Real ka[4];
    int i, turn = -1;
    ka[X] = k->x; ka[Y] = k->y; ka[Z] = k->z;
    if (ka[X]==ka[Y]) {if (ka[X]==ka[Z]) turn = W; else turn = Z;}
    else {if (ka[X]==ka[Z]) turn = Y; else if (ka[Y]==ka[Z]) turn = X;}
    if (turn>=0) {
    Quat qtoz, qp;
    unsigned neg[3], win;
    Real mag[3], t;
    static Quat qxtoz = {0,SQRTHALF,0,SQRTHALF};
    static Quat qytoz = {SQRTHALF,0,0,SQRTHALF};
    static Quat qppmm = { 0.5, 0.5,-0.5,-0.5};
    static Quat qpppp = { 0.5, 0.5, 0.5, 0.5};
    static Quat qmpmm = {-0.5, 0.5,-0.5,-0.5};
    static Quat qpppm = { 0.5, 0.5, 0.5,-0.5};
    static Quat q0001 = { 0.0, 0.0, 0.0, 1.0};
    static Quat q1000 = { 1.0, 0.0, 0.0, 0.0};
    switch (turn) {
    default: return (Qt_Conj(q));
    case X: q = Qt_Mul(q, qtoz = qxtoz); swap(ka,X,Z) break;
    case Y: q = Qt_Mul(q, qtoz = qytoz); swap(ka,Y,Z) break;
    case Z: qtoz = q0001; break;
    }
    q = Qt_Conj(q);
    mag[0] = (Real)q.z*q.z+(Real)q.w*q.w-0.5;
    mag[1] = (Real)q.x*q.z-(Real)q.y*q.w;
    mag[2] = (Real)q.y*q.z+(Real)q.x*q.w;
    for (i=0; i<3; i++) if ((neg[i] = (mag[i]<0.0)) == true) mag[i] = -mag[i];
    if (mag[0]>mag[1]) {if (mag[0]>mag[2]) win = 0; else win = 2;}
    else           {if (mag[1]>mag[2]) win = 1; else win = 2;}
    switch (win) {
    case 0: if (neg[0]) p = q1000; else p = q0001; break;
    case 1: if (neg[1]) p = qppmm; else p = qpppp; cycle(ka,0) break;
    case 2: if (neg[2]) p = qmpmm; else p = qpppm; cycle(ka,1) break;
    }
    qp = Qt_Mul(q, p);
    t = Alge::sqrt(mag[win]+0.5);
    p = Qt_Mul(p, Qt_(0.0,0.0,-qp.z/t,qp.w/t));
    p = Qt_Mul(qtoz, Qt_Conj(p));
    } else {
    Real qa[4], pa[4];
    unsigned lo, hi, neg[4], par = 0;
    Real all, big, two;
    qa[0] = q.x; qa[1] = q.y; qa[2] = q.z; qa[3] = q.w;
    for (i=0; i<4; i++) {
        pa[i] = 0.0;
        if ((neg[i] = (qa[i]<0.0)) == true) qa[i] = -qa[i];
        par ^= neg[i];
    }
    /* Find two largest components, indices in hi and lo */
    if (qa[0]>qa[1]) lo = 0; else lo = 1;
    if (qa[2]>qa[3]) hi = 2; else hi = 3;
    if (qa[lo]>qa[hi]) {
        if (qa[lo^1]>qa[hi]) {hi = lo; lo ^= 1;}
        else {hi ^= lo; lo ^= hi; hi ^= lo;}
    } else {if (qa[hi^1]>qa[lo]) lo = hi^1;}
    all = (qa[0]+qa[1]+qa[2]+qa[3])*0.5;
    two = (qa[hi]+qa[lo])*SQRTHALF;
    big = qa[hi];
    if (all>two) {
        if (all>big) {/*all*/
        {int i; for (i=0; i<4; i++) pa[i] = sgn(neg[i], 0.5);}
        cycle(ka,par)
        } else {/*big*/ pa[hi] = sgn(neg[hi],1.0);}
    } else {
        if (two>big) {/*two*/
        pa[hi] = sgn(neg[hi],SQRTHALF); pa[lo] = sgn(neg[lo], SQRTHALF);
        if (lo>hi) {hi ^= lo; lo ^= hi; hi ^= lo;}
        if (hi==W) {hi = "\001\002\000"[lo]; lo = 3-hi-lo;}
        swap(ka,hi,lo)
        } else {/*big*/ pa[hi] = sgn(neg[hi],1.0);}
    }
    p.x = -pa[0]; p.y = -pa[1]; p.z = -pa[2]; p.w = pa[3];
    }
    k->x = ka[X]; k->y = ka[Y]; k->z = ka[Z];
    return (p);
}


/******* Decompose Affine Matrix *******/

/** Decompose 4x4 affine matrix A as TFRUK(U transpose), where t contains the
  * translation components, q contains the rotation R, u contains U, k contains
  * scale factors, and f contains the sign of the determinant.
  * Assumes A transforms column vectors in right-handed coordinates.
  * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
  * Proceedings of Graphics Interface 1992.
  */
template<class Real>
void DecompAffine<Real>::decomp_affine(HMatrix A, AffineParts *parts)
{
    HMatrix Q, S, U;
    Quat p;
    Real det;
    parts->t = Qt_(A[X][W], A[Y][W], A[Z][W], 0);
    det = polar_decomp(A, Q, S);
    if (det<0.0) {
    mat_copy(Q,=,-Q,3);
    parts->f = -1;
    } else parts->f = 1;
    parts->q = Qt_FromMatrix(Q);
    parts->k = spect_decomp(S, U);
    parts->u = Qt_FromMatrix(U);
    p = snuggle(parts->u, &parts->k);
    parts->u = Qt_Mul(parts->u, p);
}

/******* Invert Affine Decomposition *******/

/* Compute inverse of affine decomposition.
  */
template<class Real>
void DecompAffine<Real>::invert_affine(AffineParts *parts, AffineParts *inverse)
{
    Quat t, p;
    inverse->f = parts->f;
    inverse->q = Qt_Conj(parts->q);
    inverse->u = Qt_Mul(parts->q, parts->u);
    inverse->k.x = (parts->k.x==0.0) ? 0.0 : 1.0/parts->k.x;
    inverse->k.y = (parts->k.y==0.0) ? 0.0 : 1.0/parts->k.y;
    inverse->k.z = (parts->k.z==0.0) ? 0.0 : 1.0/parts->k.z;
    inverse->k.w = parts->k.w;
    t = Qt_(-parts->t.x, -parts->t.y, -parts->t.z, 0);
    t = Qt_Mul(Qt_Conj(inverse->u), Qt_Mul(t, inverse->u));
    t = Qt_(inverse->k.x*t.x, inverse->k.y*t.y, inverse->k.z*t.z, 0);
    p = Qt_Mul(inverse->q, inverse->u);
    t = Qt_Mul(p, Qt_Mul(t, Qt_Conj(p)));
    inverse->t = (inverse->f>0.0) ? t : Qt_(-t.x, -t.y, -t.z, 0);
}


//==============================================================================================================
//==============================================================================================================

template<class Real, int O>
void Matrix<4,4,Real,O>::decomposeSkew( optional<Vec3&> trans, optional<Quat&> rot,
                                        optional<Vec3&> scale, optional<Quat&> skew) const
{
    DecompAffine::HMatrix hmat;
    this->toArray(&hmat[0][0]);
    DecompAffine::AffineParts parts;
    DecompAffine::decomp_affine(hmat, &parts);
    if (trans) trans = Vec3(Real(parts.t.x), Real(parts.t.y), Real(parts.t.z));
    if (rot) rot = Quat(Real(parts.q.x), Real(parts.q.y), Real(parts.q.z), Real(parts.q.w));
    if (scale) scale = parts.f >= 0 ?   Vec3(Real(parts.k.x), Real(parts.k.y), Real(parts.k.z)) :
                                        Vec3(Real(-parts.k.x), Real(-parts.k.y), Real(-parts.k.z));
    if (skew) skew = Quat(Real(parts.u.x), Real(parts.u.y), Real(parts.u.z), Real(parts.u.w));
}

template class Matrix<4,4,Float>;
template class Matrix<4,4,Double>;
template class Matrix<4,4,Quad>;

}
