// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/StudentT.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{

template<class Real>
Real StudentT_<Real>::next() const
{
    return Gaussian(getGen()).next() / Alge_d::sqrt(ChiSqr(getGen(), n).next() / n);
}

template<class Real>
Real StudentT_<Real>::pdf(Real x) const
{
    Double res;
    Double basem1 = x*x / n;
    if(basem1 < 0.125)
        res = Alge_d::exp(-Alge_d::log1p(basem1) * (1+n) / 2);
    else
        res = Alge_d::pow(1 / (1 + basem1), (n + 1) / 2);
    return res / (Alge_d::sqrt(n) * Beta(n/2, 0.5).func());
}

template<class Real>
Real StudentT_<Real>::cdf(Real x) const
{
    if (x == 0) return 0.5;
    //Calcuate prob using the incomplete beta function
    //p = BetaInc(n/2, 1/2, n/(n + x*x))
    //When x is small relative to n, rounding errors may be introduced,
    //so the 'identity' formula is used: 
    //       I[z](a,b) = 1 - I[1-z](b,a)
    // with: z = n / (n + x^2)
    // so:   1 - z = x^2 / (n + x^2)
    Real x2 = x*x;
    Real p;
    if (n > 2*x2)
    {
        Real z = x2 / (n + x2);
        p = (1 - BetaInc::calc(z, 0.5, n/2)) / 2;
    }
    else
    {
        Real z = n / (n + x2);
        p = BetaInc::calc(z, n/2, 0.5) / 2;
    }
    return x > 0 ? 1 - p : p;
}

template<class Real>
Real StudentT_<Real>::cdfComp(Real x) const
{
    return cdf(-x);
}

template<class Real>
Real StudentT_<Real>::cdfInv(Real P) const
{
    if (P <= 0) return -Real_::inf;
    if (P >= 1) return Real_::inf;
    if (P == 0.5) return 0;
    Double p = P > 0.5 ? 1 - P : P;
    Double t, x, y;
    x = BetaInc::calcInv(2*p, n/2, 0.5);
    y = 1-x;
    if (n*y > Double_::max*x)
        t = Double_::inf;
    else
        t = Alge_d::sqrt(n*y / x);
    if (P < 0.5) t = -t;
    return t;
}

template class StudentT_<Float>;
template class StudentT_<Double>;

}