// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/Binomial.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real Binomial_<Real>::next() const
{
    //From Art of Computer Programming, Vol. 2, p. 131
    Double pd = p;
    Int t = n;
    Int N = 0;

    //If t is too large then perform reductions using beta variate
    static const Int tSmall = 10;
    while (t > tSmall)
    {
        Int a = 1+(t/2);
        Int b = 1+t-a;
        Double X = Beta(getGen(), a, b).next();

        if (X >= pd)
        {
            t = a-1;
            pd /= X;
        }
        else
        {
            t = b-1;
            pd = (pd-X) / (1-X);
            N += a;
        }
    }

    //Use basic variate generator when t is small
    for (Int i = 0; i < t; ++i)
        if (Uniform::nextStd(getGen()) < pd)
            N++;

    return N;
}

template<class Real>
Real Binomial_<Real>::pdf(Real x) const
{
    Double xd = Alge::floor(x);
    if (xd < 0 || xd > n) return 0;
    Double l = GammaFunc::gammaLn(n+1) - GammaFunc::gammaLn(xd+1) - GammaFunc::gammaLn(n-xd+1)
                + xd * Alge_d::log(Double(p)) + (n-xd) * Alge_d::log(Double(1 - p));
    return (l < -Alge_d::logMax) ? 0 : Alge_d::exp(l);
}

template<class Real>
Real Binomial_<Real>::cdf(Real x) const
{
    x = Alge::floor(x);
    if (x < 0)
        return 0;
    if (x >= n)
        return 1;
    return Beta(Double(n-x), Double(x+1)).cdf(Double(1-p));
}

template<class Real>
Real Binomial_<Real>::cdfInv(Real y) const
{
    if (y <= 0)
        return 0;
    if (y >= 1)
        return n;
    return this->cdfInvFind(y, 0, n, true);
}

template<class Real>
Real Binomial_<Real>::trialProb(Int x, Int n, Real P)
{
    if (P < 0 || P > 1)
        return 0;
    if (x < 0 || n <= x)
        return 0;

    Double p = P;
    Double nd = n - x;
    if (x == 0)
    {
        if (p > 0.8)
            return -Alge_d::expm1( Alge_d::log1p(p-1) / nd );
        else
            return 1 - Alge_d::pow(p, 1/nd );
    }
    else
    {
        Double xd = x + 1;
        Double p = Beta(nd, xd).cdf(0.5);
        if (p > 0.5)
            return Beta(xd, nd).cdfInv(1-p);
        else
            return 1 - Beta(nd, xd).cdfInv(p);
    }
}

template class Binomial_<Float>;
template class Binomial_<Double>;

}