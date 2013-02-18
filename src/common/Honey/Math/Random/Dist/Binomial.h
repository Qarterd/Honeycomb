// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Beta.h"

namespace honey
{

/// Generate a random integer variate from a binomial distribution
/**
  * The result _x_ is the integer number of successes in _n_ independent trials, where each trial has success probability _p_.
  *  
  * Example: Toss a coin (head _p_ = 0.5) 50 times (_n_) and count number of heads (_x_)
  * 
  * Probability density function:
  *
  * \f$\displaystyle p(x,n,p) = \binom{n}{x} p^x (1-p)^{n-x} \f$
  * where \f$\displaystyle \binom{n}{k} = \frac{n!}{k!(n-k)!} \f$ is the _choose_ function.
  * 
  * \param  n   Number of trials. Range > 0
  * \param  p   Success probability. Range [0,1]
  * \retval x   Number of successes. Range [0,n]
  */
template<class Real>
class Binomial_ : public RandomDist<Real>
{
    typedef RandomDist<Real> Super;
    RandomDist_imports();
    template<class> friend class Binomial_;
    typedef typename Numeral<Real>::Int Int;
    typedef GammaFunc_<Double> GammaFunc;
    typedef Beta_<Double> Beta;
    
public:
    Binomial_(RandomGen& gen, Int n, Real p)    : RandomDist<Real>(gen), n(n), p(p) { assert(n > 0 && p >= 0 && p <= 1); }

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                   { return n*p; }
    virtual Real variance() const               { return n*p*(1-p); }
    
    /// Calculate trial probability given all other values
    /**
      * Finds the per trial success probability _p_ such that _x_ or fewer number of successes will occur in _n_ trials,
      * with a probability of _P_
      */
    static Real trialProb(Int x, Int n, Real P);

    Int n;
    Real p;
};

typedef Binomial_<Real>      Binomial;
typedef Binomial_<Float>     Binomial_f;
typedef Binomial_<Double>    Binomial_d;

extern template class Binomial_<Float>;
extern template class Binomial_<Double>;

}
