// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/NumAnalysis/BackSub.h"
#include "Honey/Math/Random/Chacha.h"

namespace honey
{

/// Singular Value Decomposition.  Can be used to calculate the pseudo-inverse of any matrix or solve least squares problems.
/**
  * The full SVD of (m x n) matrix A is: \f$ A = U W V^T \f$
  * where the columns of (m x m) _u_ and rows of (n x n) _vt_ contain the orthogonal left and right singular vectors
  * and (m x n) _w_ contains the diagonal non-negative singular values.
  *
  * In the reduced SVD, the singular vectors beyond the smallest dimension are not computed. \n
  * So, given _w_dim_ = min(m,n), _u_ is (m x _w_dim_), and _vt_ is (_w_dim_ x n).
  *
  * Most linear systems do not require the full SVD to solve.
  *
  * Complexity: \f$ O(n^2 m) \f$, where n and m are the smaller and larger of the two dimensions
  */
template<class Real>
class Svd
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    typedef Alge_<Double>                   Alge_d;
public:
    typedef Matrix<matrix::dynamic, matrix::dynamic, Real> Matrix;
    typedef Vec<matrix::dynamic, Double>    Vec_d;
    typedef Vec<matrix::dynamic, Real>      Vec;

    enum class Mode
    {
        full,       ///< compute full SVD
        reduced     ///< compute reduced SVD
    };

    Svd() {}
    /// Calculate the SVD of matrix A
    template<class T>
    Svd(const MatrixBase<T>& a, Mode mode = Mode::reduced)      { calc(a, mode); }

    /// Calculate the SVD of matrix A
    template<class T>
    Svd& calc(const MatrixBase<T>& a, Mode mode = Mode::reduced)
    {
        sdt m = a.rows(), n = a.cols();
        _w.resize(Alge::min(m, n));
        _wd.resize(_w.size());

        if (m >= n)
        {
            _vt.resize(n, n);
            if (mode == Mode::full) a.transpose(_ut.resize(m,m).block(0,0,n,m));
            else                    a.transpose(_ut);
            jacobi(_ut, _w, _wd, _vt, _rand);
            _ut.transpose(_u);
        }
        else
        {
            //m < n
            _u.resize(m, m);
            if (mode == Mode::full) _vt.resize(n,n).block(0,0,m,n) = a;
            else                    _vt = a;
            jacobi(_vt, _w, _wd, _u, _rand);
            _u.transposeInPlace();
        }

        return *this;
    }

    /// Calculate the singular values of matrix A.  This is a fast method for when the left and right singular vectors are not needed.
    template<class T>
    Svd& calcValues(const MatrixBase<T>& a)
    {
        sdt m = a.rows(), n = a.cols();
        _w.resize(Alge::min(m, n));
        _wd.resize(_w.size());

        if (m >= n)
        {
            a.transpose(_ut);
            jacobi(_ut, _w, _wd, optnull, _rand);
        }
        else
        {
            _vt = a;
            jacobi(_vt, _w, _wd, optnull, _rand);
        }

        return *this;
    }

    /// Solve the linear system \f$ Ax = B \f$ where _A_ is the decomposed matrix. _A_ and _B_ row sizes must match.
    template<class B, class X>
    void solve(const MatrixBase<B>& b, MatrixBase<X>& x)        { _backSub.solve(_w, _u, _vt, b, x); }

    /// Calculate the inverse of A, the decomposed matrix.
    template<class T>
    void inverse(MatrixBase<T>& res)                            { _backSub.solve(_w, _u, _vt, res); }

    /// Get the singular values of the decomposition
    const Vec& w() const                                        { return _w; }
    /// Get the left singular column vectors of the decomposition
    const Matrix& u() const                                     { return _u; }
    /// Get the right singular row vectors of the decomposition
    const Matrix& vt() const                                    { return _vt; }

private:
    /// This algorithm is adapted from the OpenCV lib
    static void jacobi(Matrix& At, Vec& w, Vec_d& wd, optional<Matrix&> Vt, RandomGen& rand);

    Vec             _w;
    Vec_d           _wd;
    Matrix          _u;
    Matrix          _ut;
    Matrix          _vt;
    Chacha          _rand;
    BackSub<Real>   _backSub;
};

extern template class Svd<Float>;
extern template class Svd<Double>;
extern template class Svd<Quad>;

}
