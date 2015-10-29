// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/Matrix.h"
#include "Honey/Math/Alge/Vec/Vec4.h"

namespace honey
{

/// Back substitute to solve a linear system
template<class Real>
class BackSub
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

    /// Check if a triangular/trapezoidal matrix has full rank (ie. all vectors are linearly independent; no vector is a linear combination of others)
    template<class T>
    static bool isFullRank(const MatrixBase<T>& a)
    {
        sdt n = Alge::min(a.rows(),a.cols());
        for (sdt i : range(n))
            if (a(i,i) == 0) return false;
        return true;
    }

    /// Solve \f$ Rx = B\f$ where _R_ is an upper triangular/trapezoidal matrix. _R_ and _B_ row sizes must match. _R_ must have full rank.
    template<class B, class X>
    static void solve(const Matrix& r, const MatrixBase<B>& b, MatrixBase<X>& x)
    {
        assert(b.rows() == r.rows());
        assert(isFullRank(r));
        sdt n = Alge::min(r.rows(),r.cols());
        sdt nx = b.cols();
        x = b;
        for (sdt k = n-1; k >= 0; --k)
        {
            for (sdt j = 0; j < nx; ++j)
                x(k,j) /= r(k,k);
            for (sdt i = 0; i < k; ++i)
                for (sdt j = 0; j < nx; ++j)
                    x(i,j) -= x(k,j)*r(i,k);
        }
    }

    /// Solve \f$ Ax = B \f$ given the SVD of _A_. _A_ and _B_ row sizes must match. \f$ x = A^{-1} B = (V^T)^T W^{-1} U^T B \f$
    template<class B, class X>
    void solve(const Vec& w, const Matrix& u, const Matrix& vt, const MatrixBase<B>& b, MatrixBase<X>& x)
    {
        assert(b.rows() == u.rows());
        _buffer.resize(b.cols());
        x.resize(vt.cols(), b.cols());
        backSubSvd(w, u, vt, optional<const MatrixBase<B>&>(b), x, _buffer);
    }

    /// Solve \f$ Ax = I \f$ given the SVD of _A_. \f$ x = A^{-1} = (V^T)^T W^{-1} U^T \f$
    template<class X>
    void solve(const Vec& w, const Matrix& u, const Matrix& vt, MatrixBase<X>& x)
    {
        _buffer.resize(u.rows());
        x.resize(vt.cols(), u.rows());
        backSubSvd(w, u, vt, optional<const MatrixBase<X>&>(optnull), x, _buffer);
    }

    /// Solve \f$ Lx = B \f$ where _L_ is a lower triangular/trapezoidal matrix. _L_ and _B_ row sizes must match. _L_ must have full rank.
    template<class B, class X>
    static void solveFwd(const Matrix& l, const MatrixBase<B>& b, MatrixBase<X>& x)
    {
        assert(b.rows() == l.rows());
        assert(isFullRank(l));
        sdt n = Alge::min(l.rows(),l.cols());
        sdt nx = b.cols();
        x = b;
        for (sdt k = 0; k < n; ++k)
        {
            for (sdt j = 0; j < nx; ++j)
                x(k,j) /= l(k,k);
            for (sdt i = k+1; i < n; ++i)
                for (sdt j = 0; j < nx; ++j)
                    x(i,j) -= x(k,j)*l(i,k);
        }
    }

private:

    /* y[0:m,0:n] += diag(a[0:1,0:m]) * x[0:m,0:n] */
    template<typename T1, typename T2, typename T3>
    static void matrAXPY(sdt m, sdt n, const T1& x, sdt dx, const T2& a, sdt ai, sdt inca, T3& y, sdt dy)
    {
        typedef typename T2::Real R2;
        typedef typename T3::Real R3;
        
        sdt i, j;
        sdt xi = 0, yi = 0;
        for( i = 0; i < m; i++, xi += dx, yi += dy )
        {
            R2 s = a(ai+i*inca);
            j=0;
            //Unrolled iteration
            for(; j <= n - 4; j += 4 )
            {
                R3 t0 = (R3)(y(yi+j)   + s*x(xi+j));
                R3 t1 = (R3)(y(yi+j+1) + s*x(xi+j+1));
                y(yi+j)   = t0;
                y(yi+j+1) = t1;
                t0 = (R3)(y(yi+j+2) + s*x(xi+j+2));
                t1 = (R3)(y(yi+j+3) + s*x(xi+j+3));
                y(yi+j+2) = t0;
                y(yi+j+3) = t1;
            }
            //Tail of iteration
            for( ; j < n; j++ )
                y(yi+j) = (R3)(y(yi+j) + s*x(xi+j));
        }
    }

    /// This algorithm is adapted from the OpenCV lib
    template<class B, class X>
    static void backSubSvd(const Vec& w, const Matrix& u, const Matrix& vt,
                            optional<const MatrixBase<B>&> b, MatrixBase<X>& x, Vec_d& buffer)
    {
        Double threshold = 0;
        Double eps = Double_::epsilon*2;
        sdt m = u.rows(), n = vt.cols();
        sdt udelta0 = 1, udelta1 = u.cols();
        sdt vdelta0 = vt.cols(), vdelta1 = 1;
        sdt ui = 0, vi = 0;
        sdt ldb = b ? b->cols() : 0, ldx = x.cols();
        sdt i, j, nm = Alge::min(m, n);
        sdt nb = b ? b->cols() : m;

        x.fromZero();
    
        for( i = 0; i < nm; i++ )
            threshold += w[i];
        threshold *= eps;
    
        // v * inv(w) * uT * b
        for( i = 0; i < nm; i++, ui += udelta0, vi += vdelta0 )
        {
            Double wi = w[i];
            if( Alge_d::abs(wi) <= threshold )
                continue;
            wi = 1/wi;
        
            if( nb == 1 )
            {
                Double s = 0;
                if( b )
                    for( j = 0; j < m; j++ )
                        s += u(ui+j*udelta1) * (*b)(j*ldb);
                else
                    s = u(ui);
                s *= wi;
            
                for( j = 0; j < n; j++ )
                    x(j*ldx) = (Real)(x(j*ldx) + s*vt(vi+j*vdelta1));
            }
            else
            {
                if( b )
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = 0;
                    matrAXPY( m, nb, *b, ldb, u, ui, udelta1, buffer, 0 );
                    for( j = 0; j < nb; j++ )
                        buffer[j] *= wi;
                }
                else
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = u(ui+j*udelta1)*wi;
                }

                matrAXPY( n, nb, buffer, 0, vt, vi, vdelta1, x, ldx );
            }
        }
    }

    Vec_d   _buffer;
};

}
