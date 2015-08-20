// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/ChiSqr.h"
#include "Honey/Math/Random/Dist/Beta.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{

/// Generate a random variate from a Student's t-distribution
/**
  * The t-dist can be used to account for uncertainty when estimating a quantity (such as the mean)
  * from a small sample of a normally distributed population.  The estimation of the quantity
  * produces additive errors that the t-dist can account for.
  *
  * The shape of the curve is wider than a normal curve, allowing samples to fall further from the mean.
  * As the degrees of freedom increase, the t-distribution approaches the normal distribution.
  *
  * Probability density function:
  *
  * \f$\displaystyle p(x,n) =   \frac{\Gamma \paren*{ \frac{n+1}{2}} } {\sqrt{n\pi} \; \Gamma \paren*{ \frac{n}{2} }}
  *                             \paren*{1 + \frac{x^2}{n}}^{\!\! -\frac{n+1}{2}} \f$
  *
  * \param  n   Number of degrees of freedom. Range > 0
  * \retval x   Random variate. Range [-inf,inf]
  */
template<class Real>
class StudentT_ : public RandomDist<Real>
{
    typedef RandomDist<Real>    Super;
    RandomDist_imports();
    typedef Gaussian_<Double>   Gaussian;
    typedef Beta_<Double>       Beta;
    typedef BetaInc<Double>     BetaInc;
    typedef ChiSqr_<Double>     ChiSqr;
    typedef Random_<Real>       Random;
    
public:
    typedef typename Random::DistStats DistStats;
    typedef Vec<2,Real>     Vec2;

    StudentT_(optional<RandomGen&> gen, Real n)         : Super(gen), n(n) { assert(n > 0); }
    StudentT_(Real n)                                   : StudentT_(optnull, n) {}

    virtual Real next() const;
    virtual Real pdf(Real x) const;
    virtual Real cdf(Real x) const;
    virtual Real cdfComp(Real x) const;
    virtual Real cdfInv(Real P) const;
    virtual Real mean() const                           { return 0; }
    virtual Real variance() const                       { return n / (n - 2); }

    struct Stats
    {
        DistStats dist;         ///< Sample distribution stats
        Vec2    meanCi;         ///< Lower and upper 100*(1-alpha)% confidence interval of the mean
        Vec2    stdDevCi;       ///< Lower and upper 100*(1-alpha)% confidence interval of the standard deviation
        szt     df;             ///< Degrees of freedom
        Real    alpha;          ///< Test significance level
        int     tail;           ///< Whether test is two-tailed or lower/upper tailed
        Real    t;              ///< T-test statistic
        Real    p;              ///< Probability of observing value more extreme than t

        friend ostream& operator<<(ostream& os, const Stats& val)
        {
            int ci = 100*(1-val.alpha);
            return os           << stringstream::indentInc << "{" << endl
                                << "Dist:"              << endl << val.dist << endl
                                << "Mean CI "           << std::setw(2) << ci
                                <<           "%:    "   << val.meanCi << endl
                                << "Std Dev CI "        << std::setw(2) << ci
                                <<              "%: "   << val.stdDevCi << endl
                                << "DF:             "   << val.df << endl
                                << "t Value:        "   << val.t << endl
               << (val.tail == 0 ? "Pr > |t|:       " :
                   val.tail == 1 ? "Pr > t:         " :
                                   "Pr < t:         ")  << val.p 
                                << stringstream::indentDec << endl  << "}";
        }
    };

    /// One-sample t-test: Test the null hypothesis that the samples are from a normally distributed population with mean `mu` and unknown standard deviation.
    /**
      * \param   samples    Sample set to test
      * \param   stats      Statistics about the test may be optionally retrieved
      * \param   mu         The mean to test
      * \param   alpha      The test is performed at the (100*alpha)% significance level, default is 5%
      * \param   tail       0 = two-tailed test (true if mean is not mu)
      *                     1 = upper tail test (true if mean is > mu)
      *                     -1 = lower tail test (true if mean is < mu)
      *
      * \return             Returns true if the null hypothesis is rejected and thus the mean is not mu.
      *                     If the result is true at 5% significance then there is at most a 1 in 20 chance
      *                     that the null hypothesis was incorrectly rejected.
      */
    template<class Range>
    static bool test(const Range& samples, optional<Stats&> stats = optnull, Real mu = 0, Real alpha = 0.05, int tail = 0)
    {
        assert(tail >= -1 && tail <= 1);
        DistStats d = Random::stats(samples);
        sdt df = d.n - 1;
        StudentT_ student(df);
        Real t = (d.mean - mu) / d.stdErr;
        Real p = student.cdfComp(tail == 0 ? Alge::abs(t) : tail == 1 ? t : -t);
        if (tail == 0) p *= 2;

        if (stats)
        {
            stats->dist = d;
            Real meanCi = student.cdfInv(1 - (tail == 0 ? alpha/2 : alpha)) * d.stdErr;
            stats->meanCi = Vec2(   tail == -1 ? -Real_::inf : d.mean - meanCi,
                                    tail == 1 ? Real_::inf : d.mean + meanCi);
            stats->stdDevCi = ChiSqr(df).stdDevCi(d.stdDev, alpha).template cast<Vec2>();
            stats->df = df;
            stats->alpha = alpha;
            stats->tail = tail;
            stats->t = t;
            stats->p = p;
        }

        return p <= alpha;
    }

    struct PooledStats
    {
        DistStats dist[2];      ///< Sample distribution stats
        Real    mean;           ///< Pooled mean (1 - 2)
        Real    stdDev;         ///< Pooled standard deviation
        Real    stdErr;         ///< Pooled standard error
        Vec2    meanCi;         ///< Lower and upper 100*(1-alpha)% confidence interval of the pooled mean
        Vec2    stdDevCi;       ///< Lower and upper 100*(1-alpha)% confidence interval of the pooled standard deviation
        szt     df;             ///< Degrees of freedom
        Real    alpha;          ///< Test significance level
        int     tail;           ///< Whether test is two-tailed or lower/upper tailed
        Real    t;              ///< T-test statistic
        Real    p;              ///< Probability of observing value more extreme than t

        friend ostream& operator<<(ostream& os, const PooledStats& val)
        {
            int ci = 100*(1-val.alpha);
            return os           << stringstream::indentInc << "{" << endl
                                << "Dist 1:"            << endl << val.dist[0] << endl
                                << "Dist 2:"            << endl << val.dist[1] << endl
                                << "Pooled Diff (1 - 2):" << endl
                                << stringstream::indentInc << "{" << endl
                                << "Mean:           "   << val.mean << endl
                                << "Std Dev:        "   << val.stdDev << endl
                                << "Std Err:        "   << val.stdErr << endl
                                << "Mean CI "           << std::setw(2) << ci
                                <<           "%:    "   << val.meanCi << endl
                                << "Std Dev CI "        << std::setw(2) << ci
                                <<              "%: "   << val.stdDevCi << endl
                                << "DF:             "   << val.df << endl
                                << "t Value:        "   << val.t << endl
               << (val.tail == 0 ? "Pr > |t|:       " :
                   val.tail == 1 ? "Pr > t:         " :
                                   "Pr < t:         ")  << val.p 
                                << stringstream::indentDec << endl  << "}"
                                << stringstream::indentDec << endl  << "}";
        }
    };

    /// Two-sample t-test: Test the null hypothesis that the difference between two sample distributions is a normally distributed population with mean `mu` and unknown standard deviation.
    /**
      * This test assumes that both sample distributions have the same variance.
      *
      * \param   samples1    First sample set to test
      * \param   samples2    Second sample set to test
      * \param   stats       Statistics about the test may be optionally retrieved
      * \param   mu          The mean to test
      * \param   alpha       The test is performed at the (100*alpha)% significance level, default is 5%
      * \param   tail        0 = two-tailed test (true if mean (1-2) is not mu)
      *                     1 = upper tail test (true if mean (1-2) > mu)
      *                     -1 = lower tail test (true if mean (1-2) < mu)
      *
      * \return              Returns true if the null hypothesis is rejected and thus the mean (1-2) is not mu.
      *                     If the result is true at 5% significance then there is at most a 1 in 20 chance
      *                     that the null hypothesis was incorrectly rejected.
      */
    template<class Range, class Range2>
    static typename std::enable_if<mt::isRange<Range2>::value, bool>::type
        test(const Range& samples1, const Range2& samples2, optional<PooledStats&> stats = optnull, Real mu = 0, Real alpha = 0.05, int tail = 0)
    {
        assert(tail >= -1 && tail <= 1);
        DistStats d1 = Random::stats(samples1);
        DistStats d2 = Random::stats(samples2);

        sdt df = d1.n + d2.n - 2;
        StudentT_ student(df);
        Real mean = d1.mean - d2.mean;
        Real stdDev = Alge::sqrt(((d1.n-1)*Alge::sqr(d1.stdDev) + (d2.n-1)*Alge::sqr(d2.stdDev)) / df);
        Real stdErr = stdDev * Alge::sqrt((1/Real(d1.n)) + (1/Real(d2.n)));
        Real t = (mean - mu) / stdErr;
        Real p = student.cdfComp(tail == 0 ? Alge::abs(t) : tail == 1 ? t : -t);
        if (tail == 0) p *= 2;

        if (stats)
        {
            stats->dist[0] = d1;
            stats->dist[1] = d2;
            stats->mean = mean;
            stats->stdDev = stdDev;
            stats->stdErr = stdErr;
            Real meanCi = student.cdfInv(1 - (tail == 0 ? alpha/2 : alpha)) * stdErr;
            stats->meanCi = Vec2(   tail == -1 ? -Real_::inf : mean - meanCi,
                                    tail == 1 ? Real_::inf : mean + meanCi);
            stats->stdDevCi = ChiSqr(df).stdDevCi(stdDev, alpha).template cast<Vec2>();
            stats->df = df;
            stats->alpha = alpha;
            stats->tail = tail;
            stats->t = t;
            stats->p = p;
        }

        return p <= alpha;
    }

    Real n;
};

typedef StudentT_<Real>     StudentT;
typedef StudentT_<Float>    StudentT_f;
typedef StudentT_<Double>   StudentT_d;

extern template class StudentT_<Float>;
extern template class StudentT_<Double>;

}
