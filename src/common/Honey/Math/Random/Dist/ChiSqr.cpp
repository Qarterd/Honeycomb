// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/Dist/ChiSqr.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real ChiSqr_<Real>::next() const
{
    Double res = Alge_d::sqr(Gaussian(getGen()).next() + ((lambda > 0) ? Alge_d::sqrt(Double(lambda)) : 0));
    if (nu > 1)
        res += 2 * Gamma(getGen(), 0.5*Double(nu-1), 1).next();
    return res;
}

template<class Real>
Real ChiSqr_<Real>::pdf(Real x) const
{
    Double xd = x, nud = nu, lambdad = lambda;
    
    if(lambda == 0)
    {
        //Centralized chi-square distribution
        if (x <= 0)
            return 0;
        else
            return Alge_d::exp((nud / 2 - 1) * Alge_d::log(xd/2) - xd/2 - GammaFunc::gammaLn(nud / 2)) / 2;
    }
    
    if(x == 0)
        return 0;

    Double x2 = xd / 2;
    Double n2 = nud / 2;
    Double l2 = lambdad / 2;
    Double sum = 0;
    int k = Alge_d::trunc(l2);
    Double pois = Gamma(Double(k + 1), 1).pdf(l2) * Gamma(n2 + k, 1).pdf(x2);
    if (pois == 0)
        return 0;
    Double poisb = pois;

    static const int iterMax = 1000;
    for(int i = k; ; ++i)
    {
        sum += pois;
        if(pois / sum < Double_::epsilon)
            break;
        if(i - k >= iterMax)
            break;
        pois *= l2 * x2 / ((i + 1) * (n2 + i));
    }
    for(int i = k - 1; i >= 0; --i)
    {
       poisb *= (i + 1) * (n2 + i) / (l2 * x2);
       sum += poisb;
       if(poisb / sum < Double_::epsilon)
          break;
    }
    return sum / 2;
}

template<class Real>
Real ChiSqr_<Real>::cdf(Real x) const
{
    if(x <= 0)
        return 0;
    Double xd = x, f = nu, lambda = this->lambda / 2;
    Double tk = Gamma(f/2 + 1, 1).pdf(xd/2);
    Double vk = Alge_d::exp(-lambda);
    Double uk = vk;
    Double sum = tk * vk;
    if(sum == 0)
        return sum;

    static const int iterMax = 1000;
    Double lterm(0), term(0);
    for(int i = 1; ; ++i)
    {
        tk = tk * xd / (f + 2 * i);
        uk = uk * lambda / i;
        vk = vk + uk;
        lterm = term;
        term = vk * tk;
        sum += term;
        if(Alge_d::abs(term / sum) < Double_::epsilon && term <= lterm)
            break;
        if(i >= iterMax)
            break;
    }

    return sum;
}

template<class Real>
Real ChiSqr_<Real>::cdfInv(Real P) const
{
    Double p = P;
    Double k = nu;
    Double l = lambda;
    Double b = (l*l) / (k+3*l);
    Double c = (k + 3*l) / (k + 2*l);
    Double ff = (k + 2*l) / (c*c);
    Double max = b + c * Gamma(ff/2, 2).cdfInv(p);
    if (max < 0)
        max = Double_::smallest;
    return this->cdfInvFind(P, 0, max);
}

template<class Real>
Real ChiSqr_<Real>::test(const VecN& observed, const VecN& expected)
{
    return 1 - ChiSqr_(observed.size()-1).cdf(
        reduce(observed, expected, Real_::zero, [](Real a, Real o, Real e) { return a + Alge::sqr(o - e) / e; }));
}

template<class Real>
Real ChiSqr_<Real>::test(const MatrixN& mat)
{
    //Get the row, column, and table totals
    auto rowTot = VecN(mat.rows()).fromZero();
    auto colTot = VecN(mat.cols()).fromZero();
    for (auto i : range(mat.rows()))
        for (auto j : range(mat.cols()))
        {
            rowTot[i] += mat[i][j];
            colTot[j] += mat[i][j];
        }
    Real N = rowTot.sum();
    
    //Calculate chisqr using estimation of expected frequency
    Real chisqr = 0;
    for (auto i : range(mat.rows()))
        for (auto j : range(mat.cols()))
        {
            Real expected = rowTot[i]*colTot[j] / N;
            chisqr += Alge::sqr(mat[i][j] - expected) / expected;
        }
    //Assume degrees of freedom is (rows-1)*(cols-1)
    return 1 - ChiSqr_((mat.rows()-1)*(mat.cols()-1)).cdf(chisqr);
}

template class ChiSqr_<Float>;
template class ChiSqr_<Double>;

}