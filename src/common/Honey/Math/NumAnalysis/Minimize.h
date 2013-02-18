// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"

namespace honey
{

/// Find a local minimum of a 1-dimensional function using "Brent's method" -- bisection and inverse quadratic interpolation
template<class Real>
class Minimize
{
    typedef Alge_<Real>             Alge;

public:
    typedef function<Real (Real)>   Func;

    /**
      * \param tol          find minimum to within tolerance
      * \param levelMax     max number of bisection steps when looking for a bracketed minimum
      * \param bracketMax   max number of bisection steps when looking for the minimum within a bracket
      */
    Minimize(Real tol = Real_::zeroTol, int levelMax = 30, int bracketMax = 30)
        : _tol(tol), _levelMax(levelMax), _bracketMax(bracketMax) {}

    /// Find the minimum of a function within bounds [min,max] using `init` as an initial guess
    /**
      * On each subinterval [a,b], the values f0 = f(a), f1 = f((a+b)/2), and f2 = f(b) are examined.
      *
      * If (f0,f1,f2) is monotonic, then [a,b] is subdivided and processed on [a,(a+b)/2] and [(a+b)/2,b].
      * The max recursion depth is 'levelMax'.
      *
      * If (f0,f1,f2) is not monotonic, then two cases arise:
      * -   If f1 = min(f0,f1,f2), then (f0,f1,f2) brackets a minimum.
      *     The minimum within the bracket is found using a form of bisection called "parabolic interpolation"
      *     with max depth 'bracketMax'.
      * -   If f1 = max(f0,f1,f2), then (f0,f1,f2) brackets a maximum.
      *     The minimum search continues recursively as in the monotonic case.
      *
      * \param func
      * \param min      minimum lower bound
      * \param max      minimum upper bound
      * \param init     initial guess of minimum
      * \retval argMin  the function arg that results in the minimum
      * \retval valMin  the minimum
      */
    tuple<Real,Real> calc(const Func& func, Real min, Real max, Real init);

private:
    // Algorithm is adapted from the Wild Magic lib
    /// Called recursively to search [t0,tm] and [tm,t1]
    void min(Real t0, Real f0, Real tm, Real t1, Real f1, int level);
    /// Called when (f0,f1,f2) brackets a minimum.
    void bracketedMin(Real t0, Real f0, Real tm, Real fm, Real t1, Real f1, int level);

    Real _tol;
    int _levelMax;
    int _bracketMax;
    const Func* _func;
    Real _tMin;
    Real _fMin;
};

extern template class Minimize<Float>;
extern template class Minimize<Double>;
extern template class Minimize<Quad>;

}

