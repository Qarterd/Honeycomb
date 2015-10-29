// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/Dist/Dist.h"
#include "Honey/Math/NumAnalysis/Bisect.h"

namespace honey
{

template<class Real>
Real RandomDist<Real>::cdfInvFind(Real p, Real min, Real max, bool discrete) const
{
    auto func = [&](Real x) { return cdf(discrete ? Alge::floor(x) : x) - p; };
    Bisect<Real> bisect;
    tie(ignore,min,max) = bisect.bracket(func,min,max);
    auto res = bisect.root(func,min,max);
    return discrete ? Alge::floor(get<1>(res)) : get<1>(res);
}

template class RandomDist<Float>;
template class RandomDist<Double>;

}
