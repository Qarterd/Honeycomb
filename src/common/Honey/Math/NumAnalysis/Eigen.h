// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"
#include "Honey/Misc/Lazy.h"

namespace honey
{

/// Eigendecomposition.  Decomposes any square (n x n) symmetric matrix \f$(A = A^T)\f$ into eigenvalues and eigenvectors.
/**
  * An eigenvector of a square matrix is a non-zero vector that when multiplied by the matrix remains parallel to the original. \n
  * An eigenvector \f$\vec{v}\f$ satisfies: \f$ A \vec{v} = w \vec{v} \f$, where _w_ is a scalar that elongates or shrinks the vector.
  *
  * The eigendecomposition of a symmetric matrix _A_ is: \f$ A = V W V^T \f$
  * where _V_ is an orthonormal matrix of column eigenvectors and _W_ is a diagonal of eigenvalues.
  *
  * Complexity: \f$ O(n^3) \f$
  */
template<class Real>
class Eigen
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    typedef Alge_<Double>                   Alge_d;
public:
    typedef Matrix<matrix::dynamic, matrix::dynamic, Real> Matrix;
    typedef Vec<matrix::dynamic, Real>      Vec;

    Eigen()                                                     { init(); }
    /// Calculate the eigendecomposition of symmetric matrix A
    template<class T>
    Eigen(const MatrixBase<T>& a)                               { init(); calc(a); }

    /// Calculate the eigendecomposition of symmetric matrix A.  The eigenvalues/vectors are sorted from largest to smallest.
    template<class T>
    Eigen& calc(const MatrixBase<T>& a)
    {
        assert(a.rows() == a.cols(), "Matrix must be square and symmetric");
        int n = a.rows();
        _w.resize(n); _indR.resize(n); _indC.resize(n);
        _v.resize(n, n);
        _a = a;
        jacobi(_a, _w, _v, _indR, _indC);
        _vt.setDirty(true);
        return *this;
    }

    /// Calculate the eigenvalues of symmetric matrix A.  This is a fast method for when the eigenvectors are not needed.
    template<class T>
    Eigen& calcValues(const MatrixBase<T>& a)
    {
        assert(a.rows() == a.cols(), "Matrix must be square and symmetric");
        int n = a.rows();
        _w.resize(n); _indR.resize(n); _indC.resize(n);
        _a = a;
        jacobi(_a, _w, optnull, _indR, _indC);
        return *this;
    }

    /// Solve the linear system \f$ Ax = B \f$ where _A_ is the decomposed matrix. A and B row sizes must match.
    template<class B, class X>
    void solve(const MatrixBase<B>& b, MatrixBase<X>& x)        { _backSub.solve(_w, _v, _vt, b, x); }

    /// Calculate the inverse of A, the decomposed matrix
    template<class T>
    void inverse(MatrixBase<T>& res)                            { _backSub.solve(_w, _v, _vt, res); }

    /// Get the eigenvalues of the decomposition
    const Vec& w() const                                        { return _w; }
    /// Get the column eigenvectors of the decomposition
    const Matrix& v() const                                     { return _v; }
    /// Get the row eigenvectors \f$ V^T \f$ of the decomposition
    const Matrix& vt() const                                    { return _vt; }

private:
    void init()                                                 { _vt.setEval([&](Matrix& val) { _v.transpose(val); }); }

    /// This algorithm is adapted from the OpenCV lib
    static void jacobi(Matrix& A, Vec& W, optional<Matrix&> V, vector<int>& indR, vector<int>& indC);

    Vec             _w;
    Matrix          _v;
    lazy<Matrix>    _vt;
    Matrix          _a;
    vector<int>     _indR;
    vector<int>     _indC;
    BackSub<Real>   _backSub;
};

extern template class Eigen<Float>;
extern template class Eigen<Double>;
extern template class Eigen<Quad>;

}
