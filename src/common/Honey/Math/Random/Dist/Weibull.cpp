// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/Weibull.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real Weibull_<Real>::next() const
{
    return Double(a) * Alge_d::pow(-Alge_d::log(Uniform::nextStd(getGen())), 1 / Double(b));
}

template<class Real>
Real Weibull_<Real>::pdf(Real x) const
{
    Double xd = x, ad = a, bd = b;
    if (x < 0)
        return 0;
    else if (x == 0)
    {
        if (b == 1)
            return 1/ad;
        else
            return 0;
    }
    else if (b == 1)
        return Alge_d::exp(-xd/ad)/ad;
    else
        return (bd/ad) * Alge_d::exp(-Alge_d::pow(xd/ad, bd) + (bd - 1) * Alge_d::log(xd/ad));
}

template<class Real>
Real Weibull_<Real>::cdf(Real x) const
{
    if (x <= 0)
        return 0;
    return -Alge_d::expm1(-Alge_d::pow(Double(x) / Double(a), Double(b)));
}

template<class Real>
Real Weibull_<Real>::cdfComp(Real x) const
{
    if (x <= 0)
        return 1;
    return Alge_d::exp(-Alge_d::pow(Double(x) / Double(a), Double(b)));
}

template<class Real>
Real Weibull_<Real>::cdfInv(Real P) const
{
    if (P >= 1)
        return Real_::inf;
    else if (P <= 0)
        return 0;
    return Double(a) * Alge_d::pow(-Alge_d::log1p(Double(-P)), 1 / Double(b));
}

template class Weibull_<Float>;
template class Weibull_<Double>;

}