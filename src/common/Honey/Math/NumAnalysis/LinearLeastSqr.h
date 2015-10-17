// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/NumAnalysis/Svd.h"
#include "Honey/Math/NumAnalysis/Qrd.h"

namespace honey
{

/// Linear least squares solver
template<class Real>
class LinearLeastSqr
{
    typedef Alge_<Real>     Alge;
    typedef Svd<Real>       Svd;
    typedef Qrd<Real>       Qrd;
    typedef BackSub<Real>   BackSub;

public:
    typedef Matrix<matrix::dynamic, matrix::dynamic, Real>  Matrix;
    typedef Vec<matrix::dynamic, Real>                      Vec;

    LinearLeastSqr() {}
        
    /// Linear least squares
    /**
      * Get a best-fit solution to the system: \f$ X \vec{b} = \vec{y} \f$
      * where the rows of (m x n) _X_ and m-dim \f$\vec{y}\f$ form a system of _m_ linear equations,
      * and n-dim \f$\vec{b}\f$ contains unknowns assumed to be linearly related.
      *
      * Given the residual (error) as \f$ \vec{r} = X \vec{b} - \vec{y} \f$, the least squares approach is to minimize
      * the residual's Euclidean \f$\ell^2\f$-norm (aka. magnitude) \f$ \| \vec{r} \| \f$. \n
      * Thus, the least squared problem becomes: \f$\displaystyle \min_{\vec{b}} \left\| X \vec{b} - \vec{y} \right\|_2 \f$ \n
      * It can be shown that this minimization problem can be uniquely solved using the SVD and the pseudo-inverse.
      *
      * ### Example: Curve Fitting
      * ______________________________
      * Consider a quadratic curve: \f$ 1 + x + x^2 = y \f$ \n
      * Let's say we have a series of \f$(x,y)\f$ samples on a graph that we want to fit to our curve.
      * If we plug in our samples directly we will find the left/right-hand sides don't agree,
      * there are errors and the best we can do is minimize them.
      *
      * Let's introduce the linearly related unknowns into the curve equation: \n
      * \f$ b_0 + b_1 x + b_2 x^2 = y \f$ \n
      * We want to find the coefficients of \f$\vec{b}\f$ which minimize the errors across all samples (a best-fit curve).
      *
      * With the model formulated we must now work it into a linear system for the solver.
      * For each \f$(x,y)\f$ sample we append a row \f$(1,x,x^2)\f$ to _X_, and a row (_y_) to \f$\vec{y}\f$.
      * We now have all the parameters needed to solve for \f$\vec{b}\f$.
      *
      * \param x    A (m x n) matrix of function coefficients. In the usual case m > n, ie. there are more samples than unknowns.
      * \param y    A m-dim vector of function results
      * \param b    Result: a n-dim vector of coefficients that best approximates the solution
      */
    void calc(const Matrix& x, const Vec& y, Vec& b);

    /// Weighted linear least squares.  Each of the _m_ equations in \f$ X \vec{b} = \vec{y} \f$ has an associated weight. A relatively low weight corresponds to high uncertainty.
    /**
      * Ideally, a sample's weight should be the inverse of its variance.
      * The residuals that least squares minimizes are multiplied by the weights.
      *
      * \param x
      * \param y
      * \param w    A m-dim vector of sample weights
      * \param b
      */
    void calc(const Matrix& x, const Vec& y, const Vec& w, Vec& b);

    /// Constrained weighted linear least squares.  Get a best-fit solution to \f$ X \vec{b} = \vec{y} \f$, subject to the constraints \f$ C \vec{b} = \vec{d} \f$.
    /**
      * This is similar to solving with k additional equations that are infinitely weighted.
      *
      * \param x
      * \param y
      * \param w
      * \param c    A (k x n) matrix of constraint coefficients, where k < n.  _X_ and _C_ col sizes must match.
      * \param d    A k-dim vector of constraint results.
      * \param b
      */
    void calc(const Matrix& x, const Vec& y, const Vec& w, const Matrix& c, const Vec& d, Vec& b);

private:
    /// \name Members for normal
    /// @{
    Svd _svd;
    /// @}

    /// \name Members for weighted
    /// @{
    Matrix _x;
    Vec _y;
    Vec _w;
    /// @}

    /// \name Members for constrained
    /// @{
    Qrd _qrd;
    Matrix _ct;
    Matrix _xq;
    Matrix _rt;
    Vec _y0;
    Vec _y1;
    Vec _yTmp;
    /// @}
};

extern template class LinearLeastSqr<Float>;
extern template class LinearLeastSqr<Double>;

}

