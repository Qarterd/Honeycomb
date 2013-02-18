// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Gamma.h"

namespace honey
{

/// Generate a random variate from a noncentral chi-square \f$\chi^2\f$ distribution
/**
  * The chi-square distribution can be used to test how likely it is that an observed distribution is due to chance.
  * It can test the goodness of fit of observations to a model by testing a 'null hypothesis' that
  * the observations are tied to some expected values.
  * It can also be used to test if categorical tallies (eg. male or female for each race) within a table are related or independent.
  *
  * ### Example
  * ___________________
  * Use Pearson's chi-squared test to determine if a 6-sided die has been tampered with. \n                
  * Take samples by rolling the die and observing the frequency of the 6 independent categorical variables
  * (ie. number of times 1,2...6 appears face up) \n
  * Here the null hypothesis is that all the sides will appear face up the same number of times.
  *
  * \f$\displaystyle \chi^2 = \sum_{i=1}^6 \frac{observedFreq_i - expectedFreq_i^2}{expectedFreq_i} \f$
  *
  * This result, _X^2_, asymptotically approaches a chi-square distribution. \n
  * Let's say that the result is 12. \n
  * Next, use the Cumulative Distribution Function (CDF).  The CDF is a curve that increases from 0 to 1,
  * it tells us the probability that a sample will be <= _X^2_. \n
  * The number of degrees of freedom are the number of categorical variables (6) minus one.
  *
  * \f$ p = 1 - cdf(\chi^2, 6-1) \f$
  *
  * _p_ will be about 0.03, or 3%.  This tells us that the probability of the die samples being tied
  * to their expected values (ie. pure chance) is 3%.  Less than 5% strongly suggests that the
  * categorical variables are correlated, so the die sides are not independent, they have been tampered with.
  * _____________________
  *
  * Probability density function when \f$\lambda = 0\f$, central chi-square distribution, same as \f$\mathrm{Gamma}(\nu/2, 2)\f$:
  *
  * \f$\displaystyle p(x,\nu,\lambda) = \frac{1}{2^{\nu/2} \Gamma(\nu/2)} x^{\nu/2 - 1} e^{-x/2} \f$
  * where \f$\Gamma\f$ is the gamma function.
  *
  * Probability density function when \f$\lambda \not= 0\f$, poisson weighted mix of central chi-square distributions:
  *
  * \f$\displaystyle p(x,\nu,\lambda) = \sum_{i=0}^\infty \frac{e^{-\lambda/2} (\lambda/2)^i}{i!} \chi^2(x, \nu+2i) \f$
  *
  * \param  nu      \f$\nu\f$ Number of degrees of freedom. Range > 0
  * \param  lambda  \f$\lambda\f$ Non-centrality parameter. Range >= 0
  * \retval x       Random variate. Range [0,inf]
  */
template<class Real>
class ChiSqr_ : public RandomDist<Real>
{
    typedef RandomDist<Real> Super;
    RandomDist_imports();
    template<class> friend class ChiSqr_;
    typedef Gaussian_<Double>   Gaussian;
    typedef Gamma_<Double>      Gamma;
    typedef GammaFunc_<Double>  GammaFunc;
    
public:
    typedef Matrix<matrix::dynamic, matrix::dynamic, Real> MatrixN;
    typedef Vec<matrix::dynamic, Real>  VecN;
    typedef Vec<2,Real>                 Vec2;

    ChiSqr_(optional<RandomGen&> gen, Real nu, Real lambda = 0)     : RandomDist<Real>(gen), nu(nu), lambda(lambda) { assert(nu > 0 && lambda >= 0); }
    ChiSqr_(Real nu, Real lambda = 0)                               : ChiSqr_(optnull, nu, lambda) {}
    
    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                           { return nu + lambda; }
    virtual Real variance() const                       { return 2*nu + 4*lambda; }
    
    /// Calculate the 100*(1-alpha)% confidence interval of the standard deviation
    Vec2 stdDevCi(Real stdDev, Real alpha) const
    {
        return Vec2(stdDev * Alge::sqrt(nu / cdfInv(1-alpha/2)),
                    stdDev * Alge::sqrt(nu / cdfInv(alpha/2)));
    }

    /// Calculate the p-value for a list of observed and expected frequencies.
    /*
      * A p-value below 5% is significant, meaning the observations are probably not due to random chance.
      * The degrees of freedom is assumed to be the number of observations - 1, that is,
      * none of the observations can be predicted by knowing other observations in the set.
      */ 
    static Real test(const VecN& observed, const VecN& expected);

    /// Calculate the p-value for a 'contingency table', a matrix of frequencies for different categories.
    /**
      * A p-value below 5% is significant, meaning that deviations between the categories are probably not due to random chance.
      * All frequencies in the matrix should be at least 5 for reliable results.
      * The degrees of freedom in the matrix is assumed to be (rows-1)*(cols-1), that is, it is not possible
      * to populate any part of the matrix knowing other parts.
      */ 
    static Real test(const MatrixN& mat);

    Real nu;
    Real lambda;
};

typedef ChiSqr_<Real>      ChiSqr;
typedef ChiSqr_<Float>     ChiSqr_f;
typedef ChiSqr_<Double>    ChiSqr_d;

extern template class ChiSqr_<Float>;
extern template class ChiSqr_<Double>;

}
