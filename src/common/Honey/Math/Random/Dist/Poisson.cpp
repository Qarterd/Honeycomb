// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/Poisson.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real Poisson_<Real>::next() const
{
    //From Art of Computer Programming, Vol. 2, p. 132
    Double mud = mu;
    Int N = 0;

    //While mu is large use log method so we can quickly skip m steps
    static const Double muSmall = 10;
    while (mud > muSmall)
    {
        Int m = mud * (7./8);
        Double X = Gamma(getGen(), m, 1).next();
        if (X < mud)
        {
            N += m;
            mud -= X;
        }
        else
            return N + Binomial(getGen(), m-1, mud/X).next();
    }

    //Use basic variate generator when mu is small
    Double L = Alge_d::exp(-mud);
    Double p = 1;

    do
    {
        N++;
        p *= Uniform::nextStd(getGen());
    }
    while (p > L);

    return N-1;
}

template<class Real>
Real Poisson_<Real>::pdf(Real x) const
{
    Double xd = Alge::floor(x), mud = mu;
    if (xd < 0) return 0;
    Double l = Alge_d::log(mud) * xd - mud - GammaFunc::gammaLn(1 + xd);
    return l < -Alge_d::logMax ? 0 : Alge_d::exp(l);
}

template<class Real>
Real Poisson_<Real>::cdf(Real x) const
{
    x = Alge::floor(x);
    if(x < 0 || mu <= 0)
        return 0;
    return Gamma(Double(x+1), 1).cdfComp(Double(mu));
}

template<class Real>
Real Poisson_<Real>::cdfComp(Real x) const
{
    x = Alge::floor(x);
    if(x < 0 || mu <= 0)
        return 0;
    return Gamma(Double(x+1), 1).cdf(Double(mu));

}

template<class Real>
Real Poisson_<Real>::cdfInv(Real P) const
{
    if (mu <= 0)
        return 0;
    if (P <= 0)
        return 0;

    Double p = P, q = 1-P;
    //Estimate a maximum value for the binary search
    // mean:
    Double m = mu;
    // standard deviation:
    Double sigma = Alge_d::sqrt(m);
    // skewness
    Double sk = 1 / sigma;
    // Get the inverse of a std normal distribution:
    Double x = Gaussian().cdfInv(Alge_d::max(p, q));
    // Set the sign:
    if (p < 0.5)
        x = -x;
    Double x2 = x * x;
    // w is correction term due to skewness
    Double w = x + sk * (x2 - 1) / 6;
    w = m + sigma * w;
    Double max = w > Double_::smallest ? w : Double_::smallest;

    // Ensure a minimum size, small estimations aren't accurate
    max = Alge_d::max(10, max*2);
    if (max == Double_::inf)
        max = 0;

    return this->cdfInvFind(P, 0, max, true);
}

template<class Real>
Real Poisson_<Real>::eventMean(Int x, Real P)
{
    if (x < 0 || P < 0 || P >= 1)
        return 0;
    return Gamma(Double(x+1), 1).cdfInv(Double(1-P));
}

template class Poisson_<Float>;
template class Poisson_<Double>;

}