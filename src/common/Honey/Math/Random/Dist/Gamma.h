// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Gaussian.h"

namespace honey
{

template<class Real> class GammaFunc_;
template<class Real> class GammaInc;

/// Generate a random variate from a gamma distribution \f$\mathrm{Gamma}\f$
/**
  * The gamma distribution is a core component for most other distributions.
  * It is a distribution of the amount of time required for a number of events to occur -- the opposite of a poisson distribution.
  *
  * Example: Waiting time until death
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,\alpha,\beta) = \frac{1}{\Gamma(\alpha) \beta^\alpha} x^{\alpha-1} e^{-x / \beta} \f$
  * where \f$\Gamma\f$ is the gamma function.
  *
  * \param  a   Shape parameter alpha \f$\alpha\f$ (aka. _k_). Range > 0
  * \param  b   Scale parameter beta \f$\beta\f$ (aka. theta \f$\theta\f$). Range > 0
  * \retval x   Random variate.  Range [0,inf]
  */
template<class Real>
class Gamma_ : public RandomDist<Real>
{
    typedef RandomDist<Real>    Super;
    RandomDist_imports();
    typedef Trig_<Real>         Trig;
    typedef Gaussian_<Double>   Gaussian;
    typedef GammaFunc_<Double>  GammaFunc;
    typedef GammaInc<Double>    GammaInc;
    
public:
    Gamma_(optional<RandomGen&> gen, Real a, Real b)    : Super(gen), a(a), b(b) { assert(a > 0 && b > 0); }
    Gamma_(Real a, Real b)                              : Gamma_(optnull, a, b) {}

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfComp(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                   { return a*b; }
    virtual Real variance() const               { return a*Alge::sqr(b); }
    
    Real a;
    Real b;
};

typedef Gamma_<Real>      Gamma;
typedef Gamma_<Float>     Gamma_f;
typedef Gamma_<Double>    Gamma_d;

extern template class Gamma_<Float>;
extern template class Gamma_<Double>;


/// Class to evaluate Gamma and related functions.
template<class Real>
class GammaFunc_
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real> Alge;
public:
    /// Gamma function
    static Real gamma(Real z)                   { return Alge::exp(gammaLn(z)); }
    /// ln(gamma) Useful in arithmetic because results of gamma can be large
    static Real gammaLn(Real z);
    /// Factorial, N!. N can be any Real including fractional numbers.
    /**
      * If N is an integer and Real type is Double then function returns perfect accuracy in N range [0,20].  Otherwise result is approximate.
      */ 
    static Real factorial(Real n);
    /// ln(N!) Useful in arithmetic because results of factorial can be large
    static Real factorialLn(Real n);
    /// Number of ways of choosing m objects from n distinct objects. "N choose M".  Both N and M can be fractional.
    static Real choose(Real n, Real m)          { return Alge::exp(chooseLn(n,m)); }
    /// ln(choose()) Useful in arithmetic because results of choose can be large
    static Real chooseLn(Real n, Real m);
private:
    static Real series(Real z);
    static Real asymp(Real z);
    static Real gValue()                        { return 10.0; }
    static Real lanczos(Real z);
    static Real near1(Real z);
    static Real near2(Real z);
    static Real gt1(Real z);
    static Real gammaLn(Real z, int& sign);

    static const int factorialTableSize;
    static const Double factorialTable[];
};

typedef GammaFunc_<Real>    GammaFunc;
typedef GammaFunc_<Float>   GammaFunc_f;
typedef GammaFunc_<Double>  GammaFunc_d;

extern template class GammaFunc_<Float>;
extern template class GammaFunc_<Double>;

}
