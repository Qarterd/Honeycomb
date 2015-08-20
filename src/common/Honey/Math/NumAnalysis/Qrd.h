// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/NumAnalysis/BackSub.h"

namespace honey
{

/// QR Decomposition.  Can be used to solve least squares problems.
/**
  * The reduced QRD of (m x n) matrix _A_ with m >= n is: \f$ A = Q R \f$
  * where _Q_ is an orthogonal (m x n) matrix, and _R_ is an upper triangular (n x n) matrix.
  *
  * The full QRD is: \f$ A = Q \begin{bmatrix} R \\ 0 \end{bmatrix} \f$ where _Q_ is (m x m) and _0_ is (m-n x n).
  *
  * Most linear systems do not require the full QRD to solve.
  *
  * Complexity: \f$ O(n^2 m) \f$
  */
template<class Real>
class Qrd
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    typedef Alge_<Double>                   Alge_d;
    typedef BackSub<Real>                   BackSub;

public:
    typedef Matrix<matrix::dynamic, matrix::dynamic, Real> Matrix;

    enum class Mode
    {
        full,       ///< compute full QRD
        reduced     ///< compute reduced QRD
    };

    Qrd() {}
    /// Calculate the QRD of matrix A
    template<class T>
    Qrd(const MatrixBase<T>& a, Mode mode = Mode::reduced)      { calc(a, mode); }

    /// Calculate the QRD of matrix A
    template<class T>
    Qrd& calc(const MatrixBase<T>& a, Mode mode = Mode::reduced)
    {
        sdt m = a.rows(), n = a.cols();
        assert(m >= n);
        _h = a;
        _q.resize(m, mode == Mode::full ? m : n);
        _r.resize(n, n);
        householder(_h, _q, _r);
        return *this;
    }

    /// Check if the decomposed matrix has full rank (ie. all vectors are linearly independent; no vector is a linear combination of others)
    bool isFullRank() const                                     { return BackSub::isFullRank(_r); }

    /// Solve the linear system \f$ Ax = B \f$ where _A_ is the decomposed matrix. _A_ and _B_ row sizes must match. _A_ must have full rank.
    template<class B, class X>
    void solve(const MatrixBase<B>& b, MatrixBase<X>& x)
    {
        sdt m = _q.rows(), n = _r.rows();
        bool full = m == _q.cols();
        if (full)   _q.block(0,0,m,n).transposeMul(b, _y);
        else        _q.transposeMul(b, _y);
        BackSub::solve(_r, _y, x);
    }

    /// Get Q of the decomposition
    const Matrix& q() const                                     { return _q; }
    /// Get R of the decomposition
    const Matrix& r() const                                     { return _r; }
    /// Get the Householder column vectors that define the reflections. H is a (m x n) lower trapazoidal matrix.
    const Matrix& h() const                                     { return _h; }

private:
    /// This algorithm is adapted from the Jama lib
    static void householder(Matrix& a, Matrix& q, Matrix& r);

    Matrix          _q;
    Matrix          _r;
    Matrix          _h;
    Matrix          _y;
};

extern template class Qrd<Float>;
extern template class Qrd<Double>;
extern template class Qrd<Quad>;

}
