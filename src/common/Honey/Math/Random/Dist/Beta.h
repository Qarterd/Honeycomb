// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Gamma.h"

namespace honey
{

template<class Real> class BetaInc;

/// Generate a random variate from a beta distribution \f$\mathrm{Beta}\f$
/**
  * The beta distribution can be most easily understood as the "conjugate prior" of the binomial distribution.
  * This means that given _x_ number of passes in _n_ number of independent trials, the beta dist will give us the expected _p_ probability.
  *
  * Example:
  *     - Rule of succession (mentioned above) \f$ p(x) = \mathrm{Beta}(x+1, n-x+1) \f$
  *     - Distribution of time required to complete a task (knowing min/max/expected interval)
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,\alpha,\beta) = \frac{x^{\alpha-1} (1-x)^{\beta-1}}{\mathrm{B}(\alpha,\beta)} \f$
  * where _B_ is the beta function.
  *
  * \param   a   Shape parameter alpha \f$\alpha\f$. Range > 0
  * \param   b   Shape parameter beta \f$\beta\f$. Range > 0
  * \retval  x   Random variate. Range [0,1]
  */
template<class Real>
class Beta_ : public RandomDist<Real>
{
    typedef RandomDist<Real> Super;
    RandomDist_imports();
    typedef Gaussian_<Double>   Gaussian;
    typedef Gamma_<Double>      Gamma;
    typedef GammaFunc_<Double>  GammaFunc;
    typedef BetaInc<Double>     BetaInc;
    
public:
    Beta_(optional<RandomGen&> gen, Real a, Real b)     : RandomDist<Real>(gen), a(a), b(b) { assert(a > 0 && b > 0); }
    Beta_(Real a, Real b)                               : Beta_(optnull, a, b) {}

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const               { return a/(a+b); }
    virtual Real variance() const           { return (a*b) / (Alge::sqr(a+b)*(a+b+1)); }
    
    /// Evaluate the beta function
    Double func() const                     { return Alge_d::exp(GammaFunc::gammaLn(a) + GammaFunc::gammaLn(b) - GammaFunc::gammaLn(a + b)); }

    Real a;
    Real b;
};

typedef Beta_<Real>      Beta;
typedef Beta_<Float>     Beta_f;
typedef Beta_<Double>    Beta_d;

extern template class Beta_<Float>;
extern template class Beta_<Double>;


/// Evaluate the incomplete beta function
template<class Real>
class BetaInc
{
    typedef typename Numeral<Real>::Real_ Real_;
    typedef Alge_<Real>         Alge;
    typedef GammaFunc_<Real>    GammaFunc;
    typedef Gaussian_<Real>     Gaussian;
public:
    static Real calc(Real x, Real a, Real b);
    static Real calcInv(Real y0, Real a, Real b);
private:
    static void values(int& n_data, Real& a, Real& b, Real& x, Real& fx);
    static Real betaIn(Real x, Real p, Real q, Real beta, int& fault);
};

extern template class BetaInc<Float>;
extern template class BetaInc<Double>;

}
