// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/Dist/HyperGeo.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real HyperGeo_<Real>::next() const
{
    if (m <= 0)
        return 0;

    //Use reflection so draw count is at most half the population
    bool reflect = n >= N/2;
    Int drawCnt = reflect ? N - n : n;

    //Loop through draws and count number of successes
    Int popTotal = N;
    Int popSuc = m;
    Int sucCnt = 0;

    for (Int i = 0; i < drawCnt; ++i)
    {
        if (popTotal*Uniform::nextStd(getGen()) < popSuc)
        {
            //Success, remove a success object from population
            --popSuc;
            //Check if we have reached size of success population
            if (++sucCnt == m)
                return reflect ? m - sucCnt : sucCnt;
        }
        //Population decreases on either success or fail
        --popTotal;
    }
    return reflect ? m - sucCnt : sucCnt;
}

template<class Real>
Real HyperGeo_<Real>::pdf(Real x) const
{
    x = Alge::floor(x);
    if (x > m || x > n)
        return 0;
    if (n > N-m && x+N-m < n)
        return 0;
    return Alge_d::exp(  GammaFunc::chooseLn(m, Double(x)) +
                        GammaFunc::chooseLn(N-m, Double(n-x)) -
                        GammaFunc::chooseLn(N, n)
                        );
}

template<class Real>
Real HyperGeo_<Real>::cdf(Real x) const
{
    if (x < 0)
        return 0;

    Double xd = Alge::floor(x);
    Double Nd = N, md = m, nd = n;

    Double result = 0;
    Double mode = Alge_d::floor((nd + 1) * (md + 1) / (Nd + 2));
    if(xd < mode)
    {
        result = HyperGeo_<Double>(getGen(),N,m,n).pdf(xd);
        Double diff = result;
        Double lower_limit = Alge_d::max(0, md + nd - Nd);
        while(diff > result * Double_::epsilon)
        {
            diff = xd * (Nd+xd-md-nd) * diff / ((1+md-xd) * (1+nd-xd));
            result += diff;
            if (xd == lower_limit)
                break;
            xd -= 1;
        }
        return Alge_d::clamp(result, 0, 1);
    }
    else
    {
        Double upper_limit = Alge_d::min(nd, md);
        if (xd != upper_limit)
        {
            xd += 1;
            result = HyperGeo_<Double>(getGen(),N,m,n).pdf(xd);
            Double diff = result;
            while(xd <= upper_limit && diff > Double_::epsilon)
            {
                diff = (md - xd) * (nd - xd) * diff / ((xd + 1) * (Nd + xd + 1 - md - nd));
                result += diff;
                xd += 1;
            }
        }
        return Alge_d::clamp(1-result, 0, 1);
    }
}

template<class Real>
Real HyperGeo_<Real>::cdfInv(Real P) const
{
    if (P <= 0)
        return 0;
    if (P >= 1)
        return m;
    return this->cdfInvFind(P, 0, n, true);
}

template class HyperGeo_<Float>;
template class HyperGeo_<Double>;

}
