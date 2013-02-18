// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/NumAnalysis/Bisect.h"

namespace honey
{

template<class Real>
tuple<bool,Real,Real> Bisect<Real>::bracket(const Func& func, Real min, Real max)
{
    //Bracket the root, expand space between min and max until bracket is found
    //Ensure that max > min
    if (max > min) max = min + 1;
    //Bracket one function at a time
    bool found = false;
    for (auto _: range(_iterMax))
    {
        mt_unused(_);
        Real fmin = func(min);
        Real fmax = func(max);
                
        //Done expanding if min/max have opposite signs
        found = fmin*fmax <= 0;
        if (found) break;

        //Expand
        Real diff = (max - min) / 2;
        min -= diff;
        max += diff;
    }
    return make_tuple(found, min, max);
}

template<class Real>
tuple<bool, Real> Bisect<Real>::root(const Func& func, Real min, Real max)
{
    //Make sure root is bracketed
    Real fmin = func(min);
    Real fmax = func(max);
    if (fmin*fmax > 0) return make_tuple(false, Real(0));

    //Do a bisection search through the function until the root is found within a certain precision
    Real x = 0, fxmin = Real_::inf;
    bool found = false;
    for (auto _: range(_iterMax))
    {
        mt_unused(_);
        Real mid = (max + min) / 2;
        Real fx = func(mid);
        
        if (fmin*fx > 0)
        {
            min = mid;
            fmin = fx;
        }
        else
            max = mid;

        fx = Alge::abs(fx);
        if (fx >= fxmin) continue;
        //Best guess so far
        fxmin = fx;
        x = mid;
        if (Alge::isNearZero(fxmin, _tol)) { found = true; break; }
    }
    return make_tuple(found, x);
}

template class Bisect<Float>;
template class Bisect<Double>;
template class Bisect<Quad>;

}