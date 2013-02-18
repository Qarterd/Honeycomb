// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Binomial.h"

namespace honey
{

/// Generate a random integer variate from a poisson distribution
/**
  * A poisson distribution can be used to count the number of events in a period of time if these events are
  * independent from each other and occur at an average rate.
  *
  * The result _x_ is an integer number of events.
  *
  * Example:
  *     - Number of particles emitted by radioactive decay per minute.
  *     - Number of times a web server is accessed per minute.
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,\mu) = \frac{\mu^x e^{-\mu}}{x!} \f$
  *
  * \param  mu  \f$\mu\f$ Mean, expected number of occurances per interval. Range > 0
  * \retval x   Number of events. Result range: [0,inf]
  */
template<class Real>
class Poisson_ : public RandomDist<Real>
{
    typedef RandomDist<Real>    Super;
    RandomDist_imports();
    template<class> friend class Poisson_;
    typedef typename Numeral<Real>::Int Int;
    typedef Gaussian_<Double>   Gaussian;
    typedef Gamma_<Double>      Gamma;
    typedef GammaFunc_<Double>  GammaFunc;
    typedef Binomial_<Double>   Binomial;
    
public:
    Poisson_(RandomGen& gen, Real mu)       : Super(gen), mu(mu) { assert(mu > 0); }

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfComp(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const               { return mu; }
    virtual Real variance() const           { return mu; }
    
    /// Calculate mean given all other values
    /**
      * Finds the mean _mu_ such that _x_ or fewer number of events will occur in the interval, with a probability of _P_
      */
    static Real eventMean(Int x, Real P);

    Real mu;
};

typedef Poisson_<Real>      Poisson;
typedef Poisson_<Float>     Poisson_f;
typedef Poisson_<Double>    Poisson_d;

extern template class Poisson_<Float>;
extern template class Poisson_<Double>;

}
