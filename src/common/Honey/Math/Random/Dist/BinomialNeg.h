// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Poisson.h"

namespace honey
{

/// Generate a random integer variate from a negative binomial distribution
/**
  * The result _x_ is the integer number of failures occurring in a sequence of trials before _n_ successes,
  * where each trial has success probability _p_.
  *
  * Example: Toss a coin (head _p_ = 0.5) and count number of tails (_x_) until the 10th time a head appears (_n_)
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,n,p) = \frac{\Gamma(n+x)}{\Gamma(n) \Gamma(x+1)} p^n (1-p)^x \f$
  * where \f$\Gamma\f$ is the gamma function.
  *
  * \param  n   Number of successes until the trials are stopped. n is Real so success count can be partial. Range > 0
  * \param  p   Success probability. Range [0,1]
  * \retval x   Number of failures. Range [0,inf]
  */
template<class Real>
class BinomialNeg_ : public RandomDist<Real>
{
    typedef RandomDist<Real> Super;
    RandomDist_imports();
    template<class> friend class BinomialNeg_;
    typedef typename Numeral<Real>::Int Int;
    typedef Gaussian_<Double>   Gaussian;
    typedef Gamma_<Double>      Gamma;
    typedef GammaFunc_<Double>  GammaFunc;
    typedef Beta_<Double>       Beta;
    typedef Poisson_<Double>    Poisson;
public:
    BinomialNeg_(RandomGen& gen, Real n, Real p)    : RandomDist<Real>(gen), n(n), p(p) { assert(n > 0 && p >= 0 && p <= 1); }

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                       { return n*(1-p) / p; }
    virtual Real variance() const                   { return n*(1-p) / Alge::sqr(p); }
    
    /// Calculate trial probability given all other values
    /**
      * Finds the per trial success probability _p_ such that _x_ or fewer number of failures will occur
      * before _n_ successes, with a probability of _P_
      */
    static Real trialProb(Int x, Real n, Real P);

    Real n;
    Real p;
};

typedef BinomialNeg_<Real>      BinomialNeg;
typedef BinomialNeg_<Float>     BinomialNeg_f;
typedef BinomialNeg_<Double>    BinomialNeg_d;

extern template class BinomialNeg_<Float>;
extern template class BinomialNeg_<Double>;

}
