// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/BinomialNeg.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real BinomialNeg_<Real>::next() const
{
    //Leger's algorithm
    Double pd = p;
    return Poisson(getGen(), Gamma(getGen(), Double(n), 1).next() * (1-pd)/pd).next();
}

template<class Real>
Real BinomialNeg_<Real>::pdf(Real x) const
{
    Double pd = p, xd = Alge::floor(x), nd = n;
    return Alge_d::exp( GammaFunc::gammaLn(xd+nd) - GammaFunc::gammaLn(nd) - GammaFunc::gammaLn(xd+1) )
            * Alge_d::pow(pd, nd) * Alge_d::pow(1-pd, xd);
}

template<class Real>
Real BinomialNeg_<Real>::cdf(Real x) const
{
    Double xd = Alge::floor(x);
    if (xd < 0)
        return 0;
    return Beta(Double(n), xd+1).cdf(Double(p));
}

template<class Real>
Real BinomialNeg_<Real>::cdfInv(Real y) const
{
    if (y <= 0)
        return 0;
    if (y >= 1)
        return Real_::inf;

    Double yd = y, yq = 1-y, sf = p, sfc = 1-p, nd = n;

    if (yd <= Alge_d::pow(sf, nd))
        // p <= pdf(0) == cdf(0)
        return 0;

    //Estimate a maximum value for the binary search
    //This estimation fails if the parameters are too small
    Double max = 0;
    if (nd * nd * nd * yd * sf > 0.005)
    {
        // mean:
        Double m = nd * sfc / sf;
        Double t = Alge_d::sqrt(nd * sfc);
        // standard deviation:
        Double sigma = t / sf;
        // skewness
        Double sk = (1 + sfc) / t;
        // kurtosis:
        Double k = (6 - sf * (5+sfc)) / (nd * sfc);
        // Get the inverse of a std normal distribution:
        Double x = Gaussian().cdfInv(Alge_d::max(yd, yq));
        // Set the sign:
        if (p < 0.5)
            x = -x;
        Double x2 = x * x;
        // w is correction term due to skewness
        Double w = x + sk * (x2 - 1) / 6;
        // Add on correction due to kurtosis.
        if (nd >= 10)
            w += k * x * (x2 - 3) / 24 + sk * sk * x * (2 * x2 - 5) / -36;
        w = m + sigma * w;
        max = (w < Double_::smallest) ? Double_::smallest : w;
    }

    // Ensure a minimum size, small estimations aren't accurate
    max = Alge_d::max(10, max*2);
    if (Alge_d::isNan(max))
        max = 0;

    return this->cdfInvFind(y, 0, max, true);
}

template<class Real>
Real BinomialNeg_<Real>::trialProb(Int x, Real n, Real P)
{
    if (P < 0 || P > 1)
        return 0;
    if (x < 0)
        return 0;
    return Beta(Double(n), Double(x+1)).cdfInv(Double(P));
}

template class BinomialNeg_<Float>;
template class BinomialNeg_<Double>;

}