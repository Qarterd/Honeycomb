// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/NumAnalysis/Minimize.h"
#include "Honey/Math/Alge/Vec/Vec4.h"

namespace honey
{

/// Find a local minimum of an n-dimensional function using "Powell's conjugate gradient descent method"
template<class Real, int Dim>
class MinimizeN
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef Alge_<Real>                     Alge;

public:
    typedef Vec<Dim, Real>          Vec;
    typedef function<Real (Vec)>    Func;

    static const int dim = Dim;

    /**
      * \param tol          find minimum to within tolerance
      * \param iterMax      max number of iterations for gradient descent method 
      * \param levelMax     see Minimize
      * \param bracketMax   see Minimize
      */
    MinimizeN(Real tol = Real_::zeroTol, int iterMax = 30, int levelMax = 30, int bracketMax = 30)
        : _tol(tol), _iterMax(iterMax), _minimize(tol, levelMax, bracketMax) {}

    /// Find the minimum of a function within bounds [min,max] using `init` as an initial guess
    /**
      * The algorithm starts at point `init` and searches for a minimum along each coordinate axis using the Minimize class.
      * A "conjugate" vector is then created by adding the deltas along each axis.
      * A new starting point is then found by taking the minimum along the conjugate vector.
      * The most successful axis is then replaced with the conjugate to minimize linear dependency.
      * The direction set is cycled and the algorithm repeats to `iterMax`.
      *
      * \param func
      * \param min      minimum lower bound
      * \param max      minimum upper bound
      * \param init     initial guess of minimum
      * \retval argMin  the function arg that results in the minimum
      * \retval valMin  the minimum
      */
    tuple<Vec,Real> calc(const Func& func, const Vec& min, const Vec& max, const Vec& init);

private:
    // Algorithm is adapted from the Wild Magic lib
    
    /// Clips line `v + t*dir` against the Cartesian product domain `[min, max]`
    /**
      * \param v        line point
      * \param dir      line direction
      * \param min      range lower bound
      * \param max      range upper bound
      * \retval tuple   the clipped t-interval
      */
    static tuple<Real, Real> calcDomain(const Vec& v, const Vec& dir, const Vec& min, const Vec& max);

    Real _tol;
    int _iterMax;
    Minimize<Real> _minimize;
};

extern template class MinimizeN<Float, 2>;
extern template class MinimizeN<Float, 3>;
extern template class MinimizeN<Double, 2>;
extern template class MinimizeN<Double, 3>;

}

