// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Poisson.h"

namespace honey
{

/// Generate a random integer variate from a weibull distribution
/**
  * The weibull distribution is used to measure a time-to-failure, the failure rate is proportional to a power of time (b-1).
  * This distribution is commonly used to model life mortality or manufacturing failure.
  *
  * If b < 1 then the failure rate is decreasing over time. This means that objects are failing early and quickly removing themselves from the population. \n
  * If b == 1 then the failure rate is constant.  This means there could be external factors that are causing the objects to fail. \n
  * If b > 1 then the failure rate increases with time.  This is typical for any aging process.
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,\alpha,\beta) = \frac{\beta}{\alpha} \paren*{ \frac{x}{\alpha} }^{\beta-1} e^{-(x/\alpha)^\beta} \f$
  *
  * \param  a   Scale parameter alpha \f$\alpha\f$ (aka. lambda \f$\lambda\f$). Range > 0
  * \param  b   Shape parameter beta \f$\beta\f$ (aka. _k_). Range > 0
  * \retval x   Random variate. Range [0, inf]
  */
template<class Real>
class Weibull_ : public RandomDist<Real>
{
    typedef RandomDist<Real>    Super;
    RandomDist_imports();
    typedef Gamma_<Double>      Gamma;
    typedef GammaFunc_<Double>  GammaFunc;
    typedef Poisson_<Double>    Poisson;
    
public:
    Weibull_(RandomGen& gen, Real a, Real b)    : Super(gen), a(a), b(b) { assert(a > 0 && b > 0); }

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfComp(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                   { return Double(a) * GammaFunc::gamma(1 + 1 / Double(b)); }
    virtual Real variance() const               { return Alge_d::sqr(Double(a)) * (GammaFunc::gamma(1 + 2 / Double(b)) - Alge_d::sqr(GammaFunc::gamma(1 + 1 / Double(b)))); }
    
    Real a;
    Real b;
};

typedef Weibull_<Real>      Weibull;
typedef Weibull_<Float>     Weibull_f;
typedef Weibull_<Double>    Weibull_d;

extern template class Weibull_<Float>;
extern template class Weibull_<Double>;

}
