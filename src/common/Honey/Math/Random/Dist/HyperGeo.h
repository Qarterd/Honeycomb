// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Beta.h"

namespace honey
{

/// Generate a random integer variate from a hypergeometric distribution
/**
  * Describes the probability of a number of successes in _n_ draws *without* replacement.
  * This is similar to the binomial distribution, which describes successes in _n_ draws *with* replacement.
  *
  * The result _x_ is the integer number of successful draws (ie. tagged objects drawn),
  * in _n_ draws from a population of _N_ objects, of which _m_ objects are tagged.
  *
  * Example: An urn contains 100 marbles (_N_), of which 20 are white (_m_).
  *         Draw 50 marbles (_n_) and count number of white marbles (_x_) drawn.
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,n,m,N) = \frac{C(m,x) C(N-m, n-x)}{C(N,m)} \f$
  * where _C_ is the _choose_ function.
  *
  * \param  N   Size of object population. Range > 0
  * \param  m   Number of tagged objects. Range [0,N]
  * \param  n   Number of draws. Range [1,N]
  * \retval x   Number of tagged objects drawn. Range [0,N]
  */
template<class Real>
class HyperGeo_ : public RandomDist<Real>
{
    typedef RandomDist<Real>    Super;
    RandomDist_imports();
    template<class> friend class HyperGeo_;
    typedef typename Numeral<Real>::Int Int;
    typedef GammaFunc_<Double>  GammaFunc;
    
public:
    HyperGeo_(RandomGen& gen, Int N, Int m, Int n) :
        Super(gen), N(N), m(m), n(n) { assert(N > 0 && m >= 0 && m <= N && n > 0 && n <= N); }

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                           { return n*m / Real(N); }
    virtual Real variance() const                       { Real Nf = N; return n * (m/Nf) * (1 - m/Nf) * (Nf-n) / (Nf-1); }

    Int N;
    Int m;
    Int n;
};

typedef HyperGeo_<Real>      HyperGeo;
typedef HyperGeo_<Float>     HyperGeo_f;
typedef HyperGeo_<Double>    HyperGeo_d;

extern template class HyperGeo_<Float>;
extern template class HyperGeo_<Double>;

}
