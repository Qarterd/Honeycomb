// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Random.h"

namespace honey
{

/// Monte Carlo (random-based) method to estimate the interval of confidence in a function's result, when that function operates on samples from a complex or unknown distribution.
/**
  * If a function operates on sample data from a distribution, it will produce confident answers with little variation if it has enough samples to work with.
  * For example, a function to find the mean of the number of heads per 10 coin tosses would operate on a large set coin toss samples, and the more samples,
  * the more the mean approaches a stable value -- 5 (the mean of a binomial distribution with N=10, p=0.5)
  * But what if the coin is rigged?  The distribution becomes unknown, thus the approaching mean is unknown.
  *
  * The bootstrap method uses the empirical data available (the toss samples) to estimate the stability of the function result (the mean).
  * This stability is expressed as a 95% confidence interval, which says that the mean of any set of toss samples will lie within the interval 95% of the time.
  * It's important to note that the confidence interval is an estimation -- the more samples provided, the more accurate and tighter the interval.
  *
  * \tparam SampleT     Bootstrap sample data type.
  * \tparam Dim         Dimension of the functor result.
  * \see Bootstrap()
  * 
  * Algorithm from "Better Bootstrap Confidence Intervals", Efron, 1987.
  */
template<class SampleT, sdt Dim = 1, class Real__ = Real>
class Bootstrap
{
public:
    typedef Real__                              Real;
private:
    typedef typename Numeral<Real>::Real_       Real_;
    typedef typename Real_::DoubleType          Double_;
    typedef typename Double_::Real              Double;
    typedef Alge_<Real>                         Alge;
    typedef Interp_<Real>                       Interp;
    typedef Gaussian_<Real>                     Gaussian;

public:
    static const sdt dim                        = Dim;
    typedef Vec<dim,Real>                       Vec;
    typedef vector<const SampleT*>              SampleList;
    typedef function<Vec (const SampleList&)>   Func;

    /// Constructor, set up constants for all calculation calls
    /**
      * The functor operates on a selection of the original samples, and returns a vector of dimension Dim.
      * For example, a coin toss mean functor would count the number of heads per 10 bool samples,
      * and return the average count (a Real) in the form of a 1D vector.
      *
      * \param func             Functor to process samples
      * \param gen              Random generator
      * \param samples          Sample data to bootstrap.
      * \param alpha            The non-confidence of the interval, usually 5%, which gives a 95% confidence interval.
      * \param bootSampleCount  The number of function samples to take for estimating the interval.
      *                         The higher the better, but above 100 is excessive.
      *                         The total functor call count is: bootSampleCount + samples.size()
      */
    Bootstrap(const Func& func, RandomGen& gen, const vector<SampleT>& samples, Real alpha = 0.05, szt bootSampleCount = 50) :
        func(func), gen(gen), samples(samples), alpha(alpha), bootSampleCount(bootSampleCount),
        _lower(0), _upper(0), _progress(0), sampleTotal(0), idx(0),
        bootRes(nullptr), jackRes(nullptr), jackMean(0)
    {
        if (samples.size() == 0) return;
        bootRes.set(new Vec[bootSampleCount]);
        bootSamples.resize(samples.size());
        jackRes.set(new Vec[samples.size()]);
    }

    ~Bootstrap() {}

    /// Perform bootstrap calculation. The calculation can be split up over multiple calls.
    /**
      * When the calculation is complete (progress == 1), the confidence interval for each dimension of the
      * result vector can be retrieved with lower() and upper().
      *
      * \param progressDelta    Percentage of progress to complete in this call. Range [0,1]
      */
    void calc(Real progressDelta = 1);

    /// Current progress of calculation, from 0 (start) to 1 (complete).
    Real progress() const                                                       { return _progress; }
    /// Get lower bound of confidence interval (calculation must be complete)
    const Vec& lower() const                                                    { return _lower; }
    /// Get upper bound of confidence interval (calculation must be complete)
    const Vec& upper() const                                                    { return _upper; }

    /// Functor to estimate the sample mean
    struct meanFunc
    {
        Vec operator()(const SampleList& samples)
        {
            Vec sum = reduce(samples, Vec().fromZero(), [](const Vec& a, const SampleT* e) { return a + *e; });
            return samples.size() > 0 ? sum / (Real)samples.size() : Vec().fromZero();
        }
    };

    /// Functor to estimate the sample variance.  This is the unbiased estimator (mean over all possible samples is the population variance)
    struct varianceFunc
    {
        Vec operator()(const SampleList& samples)
        {
            Vec mean = meanFunc()(samples);
            Vec sumDev = reduce(samples, Vec().fromZero(), [&](const Vec& a, const SampleT* e) -> Real { return a + (*e - mean).elemSqr(); });
            return samples.size() > 1 ? sumDev / (Real)(samples.size()-1) : Vec().fromZero();
        }
    };

private:
    //Input
    Func func;
    RandomGen& gen;
    const vector<SampleT>& samples;
    Real alpha;
    szt bootSampleCount;

    //Output
    Vec _lower;
    Vec _upper;

    //Progress
    Real _progress;
    sdt sampleTotal;
    sdt idx;

    //Calc locals
    UniquePtr<Vec[]> bootRes;
    SampleList bootSamples;
    Vec origRes;
    UniquePtr<Vec[]> jackRes;
    Vec jackMean;
};

template<class SampleT, sdt Dim, class Real>
void Bootstrap<SampleT,Dim,Real>::calc(Real progressDelta)
{
    if (_progress == 1)
        return;

    _progress = Alge::min(_progress + progressDelta, 1);
    sdt sampleAcc = _progress*(bootSampleCount + samples.size()) - sampleTotal;
    sampleTotal += sampleAcc;

    if (samples.size() == 0)
        return;

    sdt sampleCount = 0;

    if (sampleTotal - sampleAcc < bootSampleCount)
    {
        //Get function result with bootstrap samples (random sampling with replacement)
        for (; idx < bootSampleCount; ++idx, ++sampleCount)
        {
            if (sampleCount >= sampleAcc)
                return;
            //Build bootstrap samples
            for (szt i = 0; i < samples.size(); ++i)
                bootSamples[i] = &samples[Discrete_d(gen, 0, samples.size()-1).next()];
            //Add bootstrap result
            bootRes[idx] = func(const_cast<const SampleList&>(bootSamples));
        }

        //Get function result with original sample data
        for (szt i = 0; i < samples.size(); ++i)
            bootSamples[i] = &samples[i];
        origRes = func(const_cast<const SampleList&>(bootSamples));

        idx = 0;
    }

    //Get function result with jackknife samples (every sample is omitted once)
    //and calc the jackknife estimator (mean), which is used to estimate bias/variance
    for (; idx < samples.size(); ++idx, ++sampleCount)
    {
        if (sampleCount >= sampleAcc)
            return;
        //Erase one sample
        bootSamples.erase(bootSamples.begin() + idx);
        //Add jackknife result
        jackRes[idx] = func(const_cast<const SampleList&>(bootSamples));
        jackMean += jackRes[idx];
        //Re-insert sample
        bootSamples.insert(bootSamples.begin() + idx, &samples[idx]);
    }
    jackMean /= Real(samples.size());

    vector<Real> ecdf(bootSampleCount);

    for (sdt i = 0; i < Vec::s_size; ++i)
    {
        //Bias, also build empirical cdf (sorted boot results)
        Real sumLess = 0;
        Real sumEq = 0;
        for (szt j = 0; j < bootSampleCount; ++j)
        {
            Real val = bootRes[j][i];
            if (val < origRes[i])
                sumLess++;
            else if (val == origRes[i])
                sumEq++;
            ecdf[j] = val;
        }
        Real z = Gaussian().cdfInv((sumLess + sumEq/2) / bootSampleCount);
        std::sort(ecdf.begin(), ecdf.end());

        //Acceleration
        Real sumDevSqr = 0;
        Real sumDevCube = 0;
        for (szt j = 0; j < samples.size(); ++j)
        {
            Real dev = jackMean[i] - jackRes[j][i];
            Real sqr = Alge::sqr(dev);
            sumDevSqr += sqr;
            sumDevCube += sqr*dev;
        }
        Real acc = (sumDevCube / Alge::pow(sumDevSqr, 1.5)) / 6;

        //Apply bias+acc correction
        Real za = Gaussian().cdfInv(alpha / 2);
        Real idxLower = Gaussian().cdf(z + (z+za) / (1-acc*(z+za))) * (ecdf.size()-1);
        Real idxUpper = Gaussian().cdf(z + (z-za) / (1-acc*(z-za))) * (ecdf.size()-1);
        
        //Empirical cdf, interpolate fractional index
        _lower[i] = idxLower < ecdf.size()-1 ? Interp::linear(Alge::frac(idxLower), ecdf[idxLower], ecdf[idxLower+1]) : ecdf[idxLower];
        _upper[i] = idxUpper < ecdf.size()-1 ? Interp::linear(Alge::frac(idxUpper), ecdf[idxUpper], ecdf[idxUpper+1]) : ecdf[idxUpper];
    }

    //Calculation is complete
    _progress = 1;
}

}

