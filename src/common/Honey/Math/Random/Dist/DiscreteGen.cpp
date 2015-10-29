// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/Dist/DiscreteGen.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{

template<class Real>
DiscreteGen_<Real>::DiscreteGen_(RandomGen& gen, List pdf) :
    RandomDist<Real>(gen)
{
    _mean = 0;
    _variance = 0;
    if (!pdf.size()) return;
    _pdf.resize(pdf.size()); 
    
    Double total = 0;
    for (szt i = 0; i < _pdf.size(); ++i)
    {
        _pdf[i] = pdf[i];
        //Calc mean
        _mean += i*_pdf[i];
        //Count total for normalization
        total += _pdf[i];
    }

    assert(total > 0);
    _mean /= total;
    _cdf.resize(_pdf.size());

    /**
      * Build a lookup table of size N = PdfSize so that variate generation is constant time O(1).
      * Every index in the lookup table has a uniform chance of P = 1 / N because the index is selected with: Uniform().next() * N
      * We need to convert this uniform chance per table index into a weighted chance per table index.
      *
      * The strategy is to cram 2 probability weights into each table index:
      *     - First we select a random index with Uniform().next() * N. Lets say the result is 10.3, so we use index 10.
      *     - We then use the left-over fractional component (0.3) to select one of the two probablities at index 10.
      *
      * Since the weights are normalized between 0 and 1 and add up to 1, there must be at least one weight < P, and at least one weight > P
      *
      * We cram 2 weights into an index by first dropping in an entire weight that is < P, then we fill up to P using a portion of a large weight that is > P.
      * The leftover portion of the large weight (which now may be < P) will go on to fill the next table index.
      *
      * In the end, the normalized weights will fill up the table slots *exactly*, giving a perfect weighted lookup table.
      */

    Double p = 1. / _pdf.size();

    vector<szt> p_under;
    vector<szt> p_over;
    _table.resize(_pdf.size());

    for (szt i = 0; i < _pdf.size(); ++i)
    {
        //Normalize PDF
        _pdf[i] /= total;

        //Build the cdf by accumulating probs
        _cdf[i] = i > 0 ? _cdf[i-1] + _pdf[i] : _pdf[i];
        
        //Calc variance
        _variance += Alge_d::sqr(i-_mean) * _pdf[i];

        //Build under/over lists
        if (_pdf[i] < p)
            p_under.push_back(i);
        else
            p_over.push_back(i);
    }

    Listd weights = _pdf;

    //Loop through weights < P
    while (!p_under.empty())
    {
        szt under = p_under.back();
        p_under.pop_back();

        if (p_over.empty())
        {
            //No weight > P to help fill.
            //The algorithm guarantees that our weight must be P.
            //This should be the only weight left and we fill this index exactly.
            _table[under].bisector = 1;
            _table[under].indexTwo = under;
            continue;
        }

        //Bisect and fill up to P with large weight
        szt over = p_over.back();
        _table[under].bisector = weights[under];
        _table[under].indexTwo = over;
        weights[over] -= p - weights[under];

        //If we are under p then change lists
        if (weights[over] < p)
        {
            p_over.pop_back();
            p_under.push_back(over);
        }
        //If we are equal to p then the index is filled
        else if (weights[over] == p)
        {
            p_over.pop_back();
            _table[over].bisector = 1;
            _table[over].indexTwo = over;
        }
    }

    //Loop through remaining weights > P
    while (!p_over.empty())
    {
        //The algorithm guarantees that our weight must be P.
        //This should be the only weight left and we fill this index exactly.
        szt over = p_over.back();
        p_over.pop_back();
        _table[over].bisector = 1;
        _table[over].indexTwo = over;
    }

    _cdf[_pdf.size()-1] = 1;

}

template<class Real>
Real DiscreteGen_<Real>::next() const
{
    Double rand = Uniform::nextStd(getGen())*_pdf.size();
    szt index = rand;
    return rand - index < _table[index].bisector ? index : _table[index].indexTwo;
}

template<class Real>
Real DiscreteGen_<Real>::pdf(Real x) const
{
    x = Alge::floor(x);
    if (x < variateMin() || x > variateMax())
        return 0;
    return _pdf[x];
}

template<class Real>
Real DiscreteGen_<Real>::cdf(Real x) const
{
    x = Alge::floor(x);
    if (x < variateMin())
        return 0;
    if (x >= variateMax())
        return 1;
    return _cdf[x];
}

template<class Real>
Real DiscreteGen_<Real>::cdfInv(Real P) const
{
    if (P <= 0)
        return variateMin();
    if (P >= 1)
        return variateMax();
    return this->cdfInvFind(P, variateMin(), variateMax(), true);
}

template class DiscreteGen_<Float>;
template class DiscreteGen_<Double>;

}
