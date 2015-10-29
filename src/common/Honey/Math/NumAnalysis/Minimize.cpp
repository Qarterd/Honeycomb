// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/Minimize.h"

namespace honey
{

template<class Real>
tuple<Real,Real> Minimize<Real>::calc(const Func& func, Real t0, Real t1, Real tInit)
{
    assert(Alge::isInRange(tInit, t0, t1), "Invalid initial guess");
    _func = &func;
    _tMin = Real_::max;
    _fMin = Real_::max;

    Real f0 = (*_func)(t0);
    if (f0 < _fMin)
    {
        _tMin = t0;
        _fMin = f0;
    }

    Real f1 = (*_func)(t1);
    if (f1 < _fMin)
    {
        _tMin = t1;
        _fMin = f1;
    }

    min(t0, f0, tInit, t1, f1, 0);

    return make_tuple(_tMin, _fMin);
}

template<class Real>
void Minimize<Real>::min(Real t0, Real f0, Real tm, Real t1, Real f1, int level)
{
    if (level++ > _levelMax) return;

    Real fm = (*_func)(tm);
    if (fm < _fMin) { _tMin = tm; _fMin = fm; }

    // Test for convergence. Tolerance must scale with numbers in range.
    if (Alge::isNear(t0, t1, 2*_tol*Alge::abs(tm) + Real_::epsilon)) return;

    if ((t1 - tm)*(f0 - fm) > (tm - t0)*(fm - f1))
    {
        // The quadratic fit has positive second derivative at the midpoint
        if (f1 > f0)
        {
            if (fm >= f0)
                // Increasing, repeat on [t0,tm]
                min(t0, f0, (t0 + tm)/2, tm, fm, level);
            else
                // Not monotonic, have a bracket
                bracketedMin(t0, f0, tm, fm, t1, f1, level);
        }
        else if (f1 < f0)
        {
            if (fm >= f1)
                // Decreasing, repeat on [tm,t1]
                min(tm, fm, (tm + t1)/2, t1, f1, level);
            else
                // Not monotonic, have a bracket
                bracketedMin(t0, f0, tm, fm, t1, f1, level);
        }
        else
        {
            // Constant, repeat on [t0,tm] and [tm,t1]
            min(t0, f0, (t0 + tm)/2, tm, fm, level);
            min(tm, fm, (tm + t1)/2, t1, f1, level);
        }
    }
    else
    {
        // The quadratic fit has a nonpositive second derivative at the midpoint
        if (f1 > f0)
            // Repeat on [t0,tm]
            min(t0, f0, (t0 + tm)/2, tm, fm, level);
        else if ( f1 < f0 )
            // Repeat on [tm,t1]
            min(tm, fm, (tm + t1)/2, t1, f1, level);
        else
        {
            // Repeat on [t0,tm] and [tm,t1]
            min(t0, f0, (t0 + tm)/2, tm, fm, level);
            min(tm, fm, (tm + t1)/2, t1, f1, level);
        }
    }
}

template<class Real>
void Minimize<Real>::bracketedMin(Real t0, Real f0, Real tm, Real fm, Real t1, Real f1, int level)
{
    for (int i = 0; i < _bracketMax; ++i)
    {
        // Update minimum value
        if (fm < _fMin)
        {
            _tMin = tm;
            _fMin = fm;
        }

        // Test for convergence. Tolerance must scale with numbers in range.
        if (Alge::isNear(t0, t1, 2*_tol*Alge::abs(tm) + Real_::epsilon)) break;

        // Compute vertex of interpolating parabola
        Real dt0 = t0 - tm;
        Real dt1 = t1 - tm;
        Real df0 = f0 - fm;
        Real df1 = f1 - fm;
        Real tmp0 = dt0*df1;
        Real tmp1 = dt1*df0;
        Real denom = tmp1 - tmp0;
        if (Alge::abs(denom) < Real_::epsilon) break;

        Real tv = tm + 0.5*(dt1*tmp1 - dt0*tmp0)/denom;
        assert(t0 <= tv && tv <= t1, "Vertex not in interval");
        Real fv = (*_func)(tv);
        if (fv < _fMin)
        {
            _tMin = tv;
            _fMin = fv;
        }

        if (tv < tm)
        {
            if (fv < fm)
            {
                t1 = tm;
                f1 = fm;
                tm = tv;
                fm = fv;
            }
            else
            {
                t0 = tv;
                f0 = fv;
            }
        }
        else if (tv > tm)
        {
            if (fv < fm)
            {
                t0 = tm;
                f0 = fm;
                tm = tv;
                fm = fv;
            }
            else
            {
                t1 = tv;
                f1 = fv;
            }
        }
        else
        {
            // The vertex of parabola is already at middle sample point
            min(t0, f0, (t0 + tm)/2, tm, fm, level);
            min(tm, fm, (tm + t1)/2, t1, f1, level);
        }
    }
}

template class Minimize<Float>;
template class Minimize<Double>;
template class Minimize<Quad>;

}