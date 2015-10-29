// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/Matrix.h"
#include "Honey/Math/Alge/Vec/Vec4.h"

namespace honey
{

template<class Real> class Quat_;
template<class Real> class Transform_;
template<class Real> class DecompAffine;

/// 4x4 homogeneous matrix class.  Supports both affine and projective operations.
/**
  * Matrix data is in row-major format. Transforms are applied on the left of column vectors (the standard way).
  *
  * ie. To transform a column vector first by _M0_, followed by a transform of _M1_, apply: \f$ v' = M_1*(M_0*v) = M_1*M_0*v \f$
  *
  *     Matrix4::VecRow       Matrix4        Matrix4::VecCol (aka. Vec4)
  *     [ x  y  z  w  ]   | 0  1  2  3  |   | x |
  *                     * | 4  5  6  7  | * | y |
  *                       | 8  9  10 11 |   | z |
  *                       | 12 13 14 15 |   | w |
  *
  * Affine matrix translation is in the last column: \f$ T(x,y,z) = [3,7,11] \f$
  */
template<class Real, int Options>
class Matrix<4,4,Real,Options> : public MatrixBase<Matrix<4,4,Real,Options>>
{
    typedef MatrixBase<Matrix<4,4,Real,Options>> Super;
    template<class T> friend class MatrixBase;

    typedef Vec<2,Real>             Vec2;
    typedef Vec<3,Real>             Vec3;
    typedef Vec<4,Real>             Vec4;
    typedef Quat_<Real>             Quat;
    typedef Transform_<Real>        Transform;
    typedef DecompAffine<Double>    DecompAffine;
    using typename Super::Alge;
    using typename Super::Trig;
    
public:
    static const sdt s_rows = Super::s_rows;
    static const sdt s_cols = Super::s_cols;
    using typename Super::VecCol;
    using typename Super::VecRow;
    using Super::mul;
    using Super::transpose;
    using Super::transposeMul;
    using Super::mulTranspose;
    using Super::transposeMulTranspose;
    using Super::m;
    
    /// No init
    Matrix() {}

    /// Construct from values
    Matrix( Real _00, Real _01, Real _02, Real _03,
            Real _10, Real _11, Real _12, Real _13,
            Real _20, Real _21, Real _22, Real _23,
            Real _30, Real _31, Real _32, Real _33)
    {
        m( 0) = _00;    m( 1) = _01;    m( 2) = _02;    m( 3) = _03;
        m( 4) = _10;    m( 5) = _11;    m( 6) = _12;    m( 7) = _13;
        m( 8) = _20;    m( 9) = _21;    m(10) = _22;    m(11) = _23;
        m(12) = _30;    m(13) = _31;    m(14) = _32;    m(15) = _33;
    }

    /// Initialize with scalar in every element
    explicit Matrix(Real scalar)                                        { fromScalar(scalar); }
    /// Initialize from array with dimensions (rows x cols). If the array is in row-major format set rowMajor to true, otherwise set to false for column-major.
    Matrix(const Real* a, sdt rows, sdt cols, bool rowMajor = true)     { this->resize(rows, cols); this->fromArray(a, rowMajor); }
    /// Construct from quaternion
    Matrix(const Quat& q)                                               { q.toMatrix(*this); }
    /// Initialize from transform
    Matrix(const Transform& tm)                                         { fromTm(tm); }
    /// Construct from matrix of same size
    template<class T>
    Matrix(const MatrixBase<T>& rhs)                                    { operator=(rhs); }

    /// Forward to base
    template<class T>
    Matrix<s_rows, T::s_cols, Real> operator*(const T& rhs) const       { return Super::operator*(rhs); }
    Matrix operator*(Real rhs) const                                    { return Super::operator*(rhs); }

    /// Make matrix identity
    Matrix& fromIdentity()
    {
        m( 0) = 1;  m( 1) = 0;  m( 2) = 0;  m( 3) = 0;
        m( 4) = 0;  m( 5) = 1;  m( 6) = 0;  m( 7) = 0;
        m( 8) = 0;  m( 9) = 0;  m(10) = 1;  m(11) = 0;
        m(12) = 0;  m(13) = 0;  m(14) = 0;  m(15) = 1;
        return *this;
    }

    /// Init from translation, rotation, scale and skew.
    /**
      * A transform first scales, then rotates, then translates: \f$ tm = T R S \f$
      *
      * Scaling is done by first rotating into scale-space using the inverse of skew,
      * then scaling, then rotating back out of scale-space: \f$ S = U K U^{-1} \f$
      */
    Matrix& fromTrs(const Vec3& trans,              const Quat& rot = Quat::identity,
                    const Vec3& scale = Vec3::one,  const Quat& skew = Quat::identity)
    {
        rot.toMatrix(*this);

        if (skew == Quat::identity)
        {
            m( 0) *= scale.x;  m( 1) *= scale.y;  m( 2) *= scale.z;
            m( 4) *= scale.x;  m( 5) *= scale.y;  m( 6) *= scale.z;
            m( 8) *= scale.x;  m( 9) *= scale.y;  m(10) *= scale.z;
        }
        else
        {
            Matrix mScale;
            mScale.fromIdentity().setScale(scale);
            Matrix mSkew(skew);
            *this *= mSkew * mScale.mulTranspose(mSkew);
        }

        setTrans(trans);
        return *this;
    }

    /// Initialize with scalar in every element
    Matrix& fromScalar(Real f)
    {
        mt::for_<0, 16>([&](sdt i) { m(i) = f; });
        return *this;
    }

    /// Initialize from transform
    Matrix& fromTm(const Transform& tm);

    /// Construct a matrix that represents a projection onto a plane along a direction.
    /**
      * `normal` and `point` define the plane.  `dir` is the unit length projection direction.
      */
    Matrix& fromObliqueProjection(const Vec3& normal, const Vec3& point, const Vec3& dir);

    /// Construct a matrix that represents a perspective projection onto a plane
    /**
      * `normal` and `point` define the plane.  `eye` is the eye point.
      */
    Matrix& fromPerspectiveProjection(const Vec3& normal, const Vec3& point, const Vec3& eye);

    /// Construct a matrix that represents a reflection on a plane
    /**
      * `normal` and `point` define the plane.
      */
    Matrix& fromReflection(const Vec3& normal, const Vec3& point);

    /// Construct a matrix suitable for a camera.
    /**
      * The result is a right-handed orthonormal space at `eye` that points towards `at` along its +Z axis.
      * `up` is the world's up axis (usually the +Y axis).
      */
    Matrix& fromLookAt(const Vec3& eye, const Vec3& at, const Vec3& up);

    /// Assign to matrix of same size
    template<class T>
    Matrix& operator=(const MatrixBase<T>& rhs)                         { Super::operator=(rhs); return *this; }

    Matrix& mul(const Matrix& rhs, Matrix& res) const
    {
        res( 0) = m( 0)*rhs( 0) + m( 1)*rhs( 4) + m( 2)*rhs( 8) + m( 3)*rhs(12);
        res( 1) = m( 0)*rhs( 1) + m( 1)*rhs( 5) + m( 2)*rhs( 9) + m( 3)*rhs(13);
        res( 2) = m( 0)*rhs( 2) + m( 1)*rhs( 6) + m( 2)*rhs(10) + m( 3)*rhs(14);
        res( 3) = m( 0)*rhs( 3) + m( 1)*rhs( 7) + m( 2)*rhs(11) + m( 3)*rhs(15);

        res( 4) = m( 4)*rhs( 0) + m( 5)*rhs( 4) + m( 6)*rhs( 8) + m( 7)*rhs(12);
        res( 5) = m( 4)*rhs( 1) + m( 5)*rhs( 5) + m( 6)*rhs( 9) + m( 7)*rhs(13);
        res( 6) = m( 4)*rhs( 2) + m( 5)*rhs( 6) + m( 6)*rhs(10) + m( 7)*rhs(14);
        res( 7) = m( 4)*rhs( 3) + m( 5)*rhs( 7) + m( 6)*rhs(11) + m( 7)*rhs(15);

        res( 8) = m( 8)*rhs( 0) + m( 9)*rhs( 4) + m(10)*rhs( 8) + m(11)*rhs(12);
        res( 9) = m( 8)*rhs( 1) + m( 9)*rhs( 5) + m(10)*rhs( 9) + m(11)*rhs(13);
        res(10) = m( 8)*rhs( 2) + m( 9)*rhs( 6) + m(10)*rhs(10) + m(11)*rhs(14);
        res(11) = m( 8)*rhs( 3) + m( 9)*rhs( 7) + m(10)*rhs(11) + m(11)*rhs(15);

        res(12) = m(12)*rhs( 0) + m(13)*rhs( 4) + m(14)*rhs( 8) + m(15)*rhs(12);
        res(13) = m(12)*rhs( 1) + m(13)*rhs( 5) + m(14)*rhs( 9) + m(15)*rhs(13);
        res(14) = m(12)*rhs( 2) + m(13)*rhs( 6) + m(14)*rhs(10) + m(15)*rhs(14);
        res(15) = m(12)*rhs( 3) + m(13)*rhs( 7) + m(14)*rhs(11) + m(15)*rhs(15);
        return res;
    }

    Matrix&& mul(const Matrix& rhs, Matrix&& res) const                 { return move(mul(rhs, res)); }

    /// Square matrix, so multiplying a column vector on the right returns a column vector of the same dimension
    VecCol operator*(const VecCol& v) const
    {
        VecCol res;
        res.x = m( 0)*v.x + m( 1)*v.y + m( 2)*v.z + m( 3)*v.w;
        res.y = m( 4)*v.x + m( 5)*v.y + m( 6)*v.z + m( 7)*v.w;
        res.z = m( 8)*v.x + m( 9)*v.y + m(10)*v.z + m(11)*v.w;
        res.w = m(12)*v.x + m(13)*v.y + m(14)*v.z + m(15)*v.w;
        return res;
    }

    /// Assumes vector has w = 1, transforms and projects result back into w = 1
    Vec3 operator*(const Vec3& v) const
    {
        Vec3 res;
        res.x = m( 0)*v.x + m( 1)*v.y + m( 2)*v.z + m( 3);
        res.y = m( 4)*v.x + m( 5)*v.y + m( 6)*v.z + m( 7);
        res.z = m( 8)*v.x + m( 9)*v.y + m(10)*v.z + m(11);
        return res / (m(12)*v.x + m(13)*v.y + m(14)*v.z + m(15));
    }

    /// Assumes vector has (z,w) = (0,1), transforms and projects result back into w = 1
    Vec2 operator*(const Vec2& v) const
    {
        Vec2 res;
        res.x = m( 0)*v.x + m( 1)*v.y + m( 3);
        res.y = m( 4)*v.x + m( 5)*v.y + m( 7);
        return res / (m(12)*v.x + m(13)*v.y + m(15));
    }

    /// Square matrix, so multiplying a row vector on the left returns a row vector of the same dimension
    friend VecRow operator*(const VecRow& v, const Matrix& mat)
    {
        VecRow res;
        res.x = v.x*mat( 0) + v.y*mat( 4) + v.z*mat( 8) + v.w*mat(12);
        res.y = v.x*mat( 1) + v.y*mat( 5) + v.z*mat( 9) + v.w*mat(13);
        res.z = v.x*mat( 2) + v.y*mat( 6) + v.z*mat(10) + v.w*mat(14);
        res.w = v.x*mat( 3) + v.y*mat( 7) + v.z*mat(11) + v.w*mat(15);
        return res;
    }

    /// Transform vector with affine part of matrix (apply pos/rot/scale). Same as operator*() but without projection back into w = 1.
    Vec3 mulAffine(const Vec3& v) const
    {
        Vec3 res;
        res.x = m( 0)*v.x + m( 1)*v.y + m( 2)*v.z + m( 3);
        res.y = m( 4)*v.x + m( 5)*v.y + m( 6)*v.z + m( 7);
        res.z = m( 8)*v.x + m( 9)*v.y + m(10)*v.z + m(11);
        return res;
    }

    /// Transform vector with upper-left 3x3 rot/scale sub-matrix (no translation)
    Vec3 mulRotScale(const Vec3& v) const
    {
        Vec3 res;
        res.x = m( 0)*v.x + m( 1)*v.y + m( 2)*v.z;
        res.y = m( 4)*v.x + m( 5)*v.y + m( 6)*v.z;
        res.z = m( 8)*v.x + m( 9)*v.y + m(10)*v.z;
        return res;
    }

    Matrix& transpose(Matrix& res) const
    {
        res( 0)=m( 0);  res( 1)=m( 4);  res( 2)=m( 8);  res( 3)=m(12);
        res( 4)=m( 1);  res( 5)=m( 5);  res( 6)=m( 9);  res( 7)=m(13);
        res( 8)=m( 2);  res( 9)=m( 6);  res(10)=m(10);  res(11)=m(14);
        res(12)=m( 3);  res(13)=m( 7);  res(14)=m(11);  res(15)=m(15);
        return res;
    }

    Matrix&& transpose(Matrix&& res) const                              { return move(transpose(res)); }

    void transposeInPlace()
    {
        std::swap(m(1), m(4));  std::swap(m(2), m(8));  std::swap(m(3), m(12));
                                std::swap(m(6), m(9));  std::swap(m(7), m(13));
                                                        std::swap(m(11), m(14));
    }

    Matrix& transposeMul(const Matrix& rhs, Matrix& res) const
    {
        res( 0) = m( 0)*rhs( 0) + m( 4)*rhs( 4) + m( 8)*rhs( 8) + m(12)*rhs(12);
        res( 1) = m( 0)*rhs( 1) + m( 4)*rhs( 5) + m( 8)*rhs( 9) + m(12)*rhs(13);
        res( 2) = m( 0)*rhs( 2) + m( 4)*rhs( 6) + m( 8)*rhs(10) + m(12)*rhs(14);
        res( 3) = m( 0)*rhs( 3) + m( 4)*rhs( 7) + m( 8)*rhs(11) + m(12)*rhs(15);
                    
        res( 4) = m( 1)*rhs( 0) + m( 5)*rhs( 4) + m( 9)*rhs( 8) + m(13)*rhs(12);
        res( 5) = m( 1)*rhs( 1) + m( 5)*rhs( 5) + m( 9)*rhs( 9) + m(13)*rhs(13);
        res( 6) = m( 1)*rhs( 2) + m( 5)*rhs( 6) + m( 9)*rhs(10) + m(13)*rhs(14);
        res( 7) = m( 1)*rhs( 3) + m( 5)*rhs( 7) + m( 9)*rhs(11) + m(13)*rhs(15);
                    
        res( 8) = m( 2)*rhs( 0) + m( 6)*rhs( 4) + m(10)*rhs( 8) + m(14)*rhs(12);
        res( 9) = m( 2)*rhs( 1) + m( 6)*rhs( 5) + m(10)*rhs( 9) + m(14)*rhs(13);
        res(10) = m( 2)*rhs( 2) + m( 6)*rhs( 6) + m(10)*rhs(10) + m(14)*rhs(14);
        res(11) = m( 2)*rhs( 3) + m( 6)*rhs( 7) + m(10)*rhs(11) + m(14)*rhs(15);
                    
        res(12) = m( 3)*rhs( 0) + m( 7)*rhs( 4) + m(11)*rhs( 8) + m(15)*rhs(12);
        res(13) = m( 3)*rhs( 1) + m( 7)*rhs( 5) + m(11)*rhs( 9) + m(15)*rhs(13);
        res(14) = m( 3)*rhs( 2) + m( 7)*rhs( 6) + m(11)*rhs(10) + m(15)*rhs(14);
        res(15) = m( 3)*rhs( 3) + m( 7)*rhs( 7) + m(11)*rhs(11) + m(15)*rhs(15);
        return res;
    }

    Matrix&& transposeMul(const Matrix& rhs, Matrix&& res) const        { return move(transposeMul(rhs, res)); }

    Matrix& mulTranspose(const Matrix& rhs, Matrix& res) const
    {
        res( 0) = m( 0)*rhs( 0) + m( 1)*rhs( 1) + m( 2)*rhs( 2) + m( 3)*rhs( 3);
        res( 1) = m( 0)*rhs( 4) + m( 1)*rhs( 5) + m( 2)*rhs( 6) + m( 3)*rhs( 7);
        res( 2) = m( 0)*rhs( 8) + m( 1)*rhs( 9) + m( 2)*rhs(10) + m( 3)*rhs(11);
        res( 3) = m( 0)*rhs(12) + m( 1)*rhs(13) + m( 2)*rhs(14) + m( 3)*rhs(15);
                    
        res( 4) = m( 4)*rhs( 0) + m( 5)*rhs( 1) + m( 6)*rhs( 2) + m( 7)*rhs( 3);
        res( 5) = m( 4)*rhs( 4) + m( 5)*rhs( 5) + m( 6)*rhs( 6) + m( 7)*rhs( 7);
        res( 6) = m( 4)*rhs( 8) + m( 5)*rhs( 9) + m( 6)*rhs(10) + m( 7)*rhs(11);
        res( 7) = m( 4)*rhs(12) + m( 5)*rhs(13) + m( 6)*rhs(14) + m( 7)*rhs(15);
                    
        res( 8) = m( 8)*rhs( 0) + m( 9)*rhs( 1) + m(10)*rhs( 2) + m(11)*rhs( 3);
        res( 9) = m( 8)*rhs( 4) + m( 9)*rhs( 5) + m(10)*rhs( 6) + m(11)*rhs( 7);
        res(10) = m( 8)*rhs( 8) + m( 9)*rhs( 9) + m(10)*rhs(10) + m(11)*rhs(11);
        res(11) = m( 8)*rhs(12) + m( 9)*rhs(13) + m(10)*rhs(14) + m(11)*rhs(15);
                    
        res(12) = m(12)*rhs( 0) + m(13)*rhs( 1) + m(14)*rhs( 2) + m(15)*rhs( 3);
        res(13) = m(12)*rhs( 4) + m(13)*rhs( 5) + m(14)*rhs( 6) + m(15)*rhs( 7);
        res(14) = m(12)*rhs( 8) + m(13)*rhs( 9) + m(14)*rhs(10) + m(15)*rhs(11);
        res(15) = m(12)*rhs(12) + m(13)*rhs(13) + m(14)*rhs(14) + m(15)*rhs(15);
        return res;
    }

    Matrix&& mulTranspose(const Matrix& rhs, Matrix&& res) const        { return move(mulTranspose(rhs, res)); }

    Matrix& transposeMulTranspose(const Matrix& rhs, Matrix& res) const
    {
        res( 0) = m( 0)*rhs( 0) + m( 4)*rhs( 1) + m( 8)*rhs( 2) + m(12)*rhs( 3);
        res( 1) = m( 0)*rhs( 4) + m( 4)*rhs( 5) + m( 8)*rhs( 6) + m(12)*rhs( 7);
        res( 2) = m( 0)*rhs( 8) + m( 4)*rhs( 9) + m( 8)*rhs(10) + m(12)*rhs(11);
        res( 3) = m( 0)*rhs(12) + m( 4)*rhs(13) + m( 8)*rhs(14) + m(12)*rhs(15);
                    
        res( 4) = m( 1)*rhs( 0) + m( 5)*rhs( 1) + m( 9)*rhs( 2) + m(13)*rhs( 3);
        res( 5) = m( 1)*rhs( 4) + m( 5)*rhs( 5) + m( 9)*rhs( 6) + m(13)*rhs( 7);
        res( 6) = m( 1)*rhs( 8) + m( 5)*rhs( 9) + m( 9)*rhs(10) + m(13)*rhs(11);
        res( 7) = m( 1)*rhs(12) + m( 5)*rhs(13) + m( 9)*rhs(14) + m(13)*rhs(15);
                    
        res( 8) = m( 2)*rhs( 0) + m( 6)*rhs( 1) + m(10)*rhs( 2) + m(14)*rhs( 3);
        res( 9) = m( 2)*rhs( 4) + m( 6)*rhs( 5) + m(10)*rhs( 6) + m(14)*rhs( 7);
        res(10) = m( 2)*rhs( 8) + m( 6)*rhs( 9) + m(10)*rhs(10) + m(14)*rhs(11);
        res(11) = m( 2)*rhs(12) + m( 6)*rhs(13) + m(10)*rhs(14) + m(14)*rhs(15);
                    
        res(12) = m( 3)*rhs( 0) + m( 7)*rhs( 1) + m(11)*rhs( 2) + m(15)*rhs( 3);
        res(13) = m( 3)*rhs( 4) + m( 7)*rhs( 5) + m(11)*rhs( 6) + m(15)*rhs( 7);
        res(14) = m( 3)*rhs( 8) + m( 7)*rhs( 9) + m(11)*rhs(10) + m(15)*rhs(11);
        res(15) = m( 3)*rhs(12) + m( 7)*rhs(13) + m(11)*rhs(14) + m(15)*rhs(15);
        return res;
    }

    Matrix&& transposeMulTranspose(const Matrix& rhs, Matrix&& res) const   { return move(transposeMulTranspose(rhs, res)); }

    /// Gram-Schmidt orthonormalization on upper-left 3x3 submatrix. Useful for re-normalizing an orthonormal basis to eliminate rounding errors.
    void orthonormalize();

    /// Get the inverse of this matrix.  The determinant will be returned in det if specified.
    Matrix inverse(optional<Real&> det = optnull) const;
    /// The adjugate matrix is the transpose of the cofactor matrix (used in inversion)
    Matrix adjugate() const;
    /// Get the determinant
    Real determinant() const;

    /// Decompose matrix into translation, rotation, scale and skew.
    /**
      * Warning:    If skew is requested and matrix likely has skew, then decomposition is about 10 times slower!
      *             Any matrix that has a non-uniform scale likely has skew.
      * /see fromTrs()
      */
    void decompose( optional<Vec3&> trans = optnull, optional<Quat&> rot = optnull,
                    optional<Vec3&> scale = optnull, optional<Quat&> skew = optnull) const;

    /// Get translation
    Vec3 getTrans() const                                               { return Vec3(m(3), m(7), m(11)); }

    /// Set translation
    void setTrans(const Vec3& v)                                        { m(3) = v.x; m(7) = v.y; m(11) = v.z; }

    /// Get rotation. Beware: bogus if mat contains scale/skew
    Quat getRot() const                                                 { return Quat().fromMatrix(*this); }

    /// Set rotation. Beware: overwrites scale/skew
    void setRot(const Quat& q)                                          { q.toMatrix(*this, true); }

    /// Get scale. Beware: bogus if mat contains rot/skew
    Vec3 getScale() const                                               { return Vec3(m(0), m(5), m(10)); }

    /// Set scale. Beware: overwrites rotation
    void setScale(const Vec3& v, const Quat& skew = Quat::identity)
    {
        if (skew == Quat::identity)
        {
            m(0) = v.x; m(5) = v.y; m(10) = v.z;
        }
        else
        {
            auto scl = Matrix().fromIdentity();
            scl(0) = v.x;  scl(5) = v.y;  scl(10) = v.z;
            Matrix skewMat(skew);
            scl = skewMat * scl.mulTranspose(skewMat);
            m( 0) = scl( 0);  m( 1) = scl( 1);  m( 2) = scl( 2);
            m( 4) = scl( 4);  m( 5) = scl( 5);  m( 6) = scl( 6);
            m( 8) = scl( 8);  m( 9) = scl( 9);  m(10) = scl(10);
        }
    }

    /// Set uniform scale. Beware: overwrites rotation
    void setScale(Real f)                                               { setScale(Vec3(f)); }

    /// Make a tm that performs this transform first, then does a translation. ie. T * This
    Matrix& translate(const Vec3& v)                                    { Matrix tm; tm.fromIdentity(); tm.setTrans(v); return operator=(tm * *this); }
    /// Make a tm that does a translation first, then performs this transform. ie. This * T
    Matrix& preTranslate(const Vec3& v)                                 { Matrix tm; tm.fromIdentity(); tm.setTrans(v); return this->operator*=(tm); }

    /// Make a tm that performs this transform first, then does a rotation. ie. R * This
    Matrix& rotate(const Quat& q)                                       { return operator=(Matrix(q) * *this); }
    /// Make a tm that does a rotation first, then performs this transform. ie. This * R
    Matrix& preRotate(const Quat& q)                                    { return this->operator*=(Matrix(q)); }

    /// Make a tm that performs this transform first, then does a scale. ie. S * This
    Matrix& scale(const Vec3& v, const Quat& skew = Quat::identity)     { Matrix tm; tm.fromIdentity(); tm.setScale(v, skew); return operator=(tm * *this); }
    /// Uniform scale
    Matrix& scale(Real f)                                               { return scale(Vec3(f)); }

    /// Make a tm that does a scale first, then performs this transform. ie. This * S
    Matrix& preScale(const Vec3& v, const Quat& skew = Quat::identity)  { Matrix tm; tm.fromIdentity(); tm.setScale(v, skew); return this->operator*=(tm); }
    /// Uniform prescale
    Matrix& preScale(Real f)                                            { return preScale(Vec3(f)); }

protected:

    template<class Num>
    Matrix& fromColMajor(const Num* a)
    {
        m( 0) = (Real)a[ 0];  m( 1) = (Real)a[ 4];  m( 2) = (Real)a[ 8];  m( 3) = (Real)a[12];
        m( 4) = (Real)a[ 1];  m( 5) = (Real)a[ 5];  m( 6) = (Real)a[ 9];  m( 7) = (Real)a[13];
        m( 8) = (Real)a[ 2];  m( 9) = (Real)a[ 6];  m(10) = (Real)a[10];  m(11) = (Real)a[14];
        m(12) = (Real)a[ 3];  m(13) = (Real)a[ 7];  m(14) = (Real)a[11];  m(15) = (Real)a[15];
        return *this;
    }

    template<class Num>
    Num* toColMajor(Num* a) const
    {
        a[ 0] = (Num)m( 0);  a[ 1] = (Num)m( 4);  a[ 2] = (Num)m( 8);  a[ 3] = (Num)m(12);
        a[ 4] = (Num)m( 1);  a[ 5] = (Num)m( 5);  a[ 6] = (Num)m( 9);  a[ 7] = (Num)m(13);
        a[ 8] = (Num)m( 2);  a[ 9] = (Num)m( 6);  a[10] = (Num)m(10);  a[11] = (Num)m(14);
        a[12] = (Num)m( 3);  a[13] = (Num)m( 7);  a[14] = (Num)m(11);  a[15] = (Num)m(15);
        return a;
    }

private:
    ///Special decomposition for matrices with non-uniform scale and skew, much slower
    void decomposeSkew( optional<Vec3&> trans, optional<Quat&> rot,
                        optional<Vec3&> scale, optional<Quat&> skew) const;

public:
    static const Matrix zero;
    static const Matrix identity;
};

/** \cond */
/// \name Specialized for optimization
/// @{
template<class R, int Opt>
struct priv::map_impl<Matrix<4,4,R,Opt>, Matrix<4,4,R,Opt>>
{
    template<class T, class O, class Func>
    static O&& func(T&& m, O&& o, Func&& f)
    {
        mt::for_<0, 16>([&](sdt i) { o(i) = f(m(i)); });
        return forward<O>(o);
    }
};

template<class R, int Opt>
struct priv::map_impl<Matrix<4,4,R,Opt>, Matrix<4,4,R,Opt>, Matrix<4,4,R,Opt>>
{
    template<class T, class T2, class O, class Func>
    static O&& func(T&& m, T2&& rhs, O&& o, Func&& f)
    {
        mt::for_<0, 16>([&](sdt i) { o(i) = f(m(i), rhs(i)); });
        return forward<O>(o);
    }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Matrix<4,4,R,O>, Accum_>
{
    template<class T, class Accum, class Func>
    static Accum_ func(T&& m, Accum&& initVal, Func&& f)
    {
        return f(f(f(f(f(f(f(f(f(f(f(f(f(f(f(f(forward<Accum>(initVal),
                m(0)),  m(1)),  m(2)),  m(3)),
                m(4)),  m(5)),  m(6)),  m(7)),
                m(8)),  m(9)),  m(10)), m(11)),
                m(12)), m(13)), m(14)), m(15));
        
    }
};

template<class R, int O, class Accum_>
struct priv::reduce_impl<Matrix<4,4,R,O>, Accum_, Matrix<4,4,R,O>>
{
    template<class T, class T2, class Accum, class Func>
    static Accum_ func(T&& m, T2&& rhs, Accum&& initVal, Func&& f)
    {
        return f(f(f(f(f(f(f(f(f(f(f(f(f(f(f(f(forward<Accum>(initVal),
                m(0),rhs(0)),   m(1),rhs(1)),   m(2),rhs(2)),   m(3),rhs(3)),
                m(4),rhs(4)),   m(5),rhs(5)),   m(6),rhs(6)),   m(7),rhs(7)),
                m(8),rhs(8)),   m(9),rhs(9)),   m(10),rhs(10)), m(11),rhs(11)),
                m(12),rhs(12)), m(13),rhs(13)), m(14),rhs(14)), m(15),rhs(15));
    }
};
/// @}
/** \endcond */

typedef Matrix<4,4,Real>    Matrix4;
typedef Matrix<4,4,Float>   Matrix4_f;
typedef Matrix<4,4,Double>  Matrix4_d;

extern template class Matrix<4,4,Float>;
extern template class Matrix<4,4,Double>;
extern template class Matrix<4,4,Quad>;

}

#define Section_Header
#include "Honey/Math/Alge/Matrix/platform/Matrix4.h"
#undef Section_Header
