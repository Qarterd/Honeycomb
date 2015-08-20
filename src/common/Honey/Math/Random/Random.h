// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/platform/Random.h"
#include "Honey/Math/Random/Dist/Discrete.h"
#include "Honey/Math/Random/Dist/Gamma.h"
#include "Honey/Math/NumAnalysis/Interp.h"

namespace honey
{

/// Random-related methods
namespace random
{
    /// Retrieve count bytes of entropy from the host device
    inline Bytes deviceEntropy(szt count)                                   { return platform::deviceEntropy(count); }
}

/// Random-related methods
template<class Real>
class Random_
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Random_<Double>                 Random_d;
    typedef Alge_<Real>                     Alge;
    typedef Alge_<Double>                   Alge_d;
    typedef Interp_<Real>                   Interp;
    typedef Uniform_<Real>                  Uniform;
    typedef Uniform_<Double>                Uniform_d;
    typedef Gaussian_<Real>                 Gaussian;
    
    typedef Vec<2,Real>                     Vec2;
    typedef Vec<3,Real>                     Vec3;
    typedef Vec<4,Real>                     Vec4;
    typedef Quat_<Real>                     Quat;
    
public:
    Random_()                                                               : _gen(nullptr) {}
    /// Construct with random generator to use for all methods
    Random_(RandomGen& gen)                                                 : _gen(&gen) {}

    /// Set random generator to use for all methods
    void setGen(RandomGen& gen)                                             { _gen = &gen; }
    /// Get random generator
    RandomGen& getGen() const                                               { assert(_gen); return *_gen; }

    /// Generate random bool true/false
    bool boolean() const                                                    { return (Discrete_<uint32>::nextStd(getGen()) & 1) == 1; }

    /// Randomly choose count items from the list with replacement, so an item can be sampled more than once.  Result is stored in sample.
    template<class T>
    void sample(const vector<T>& list, szt count, vector<T>& sample) const
    {
        if (list.size() == 0) { sample.clear(); return; }
        sample.resize(count);
        for (szt i = 0; i < count; ++i)
            sample[i] = list[Discrete_d(getGen(), 0, list.size()-1).next()];
    }

    /// Randomly choose count items from a list without replacement, so an item can't be chosen more than once.  Results stored in `chosen`, and `unchosen` (unchosen indices).
    template<class T>
    void choose(const vector<T>& list, szt count, vector<T>& chosen, vector<szt>& unchosen) const
    {
        szt chosenSize = Alge::min(count, list.size());
        if (chosenSize == 0) { chosen.clear(); unchosen.clear(); return; }

        //Build unchosen index list
        unchosen.resize(list.size());
        for (szt i = 0; i < unchosen.size(); ++i) unchosen[i] = i;

        //Build chosen list
        chosen.resize(chosenSize);
        for (szt i = 0; i < chosenSize; ++i)
        {
            //Get random unchosen index and remove it
            szt index = Discrete_d(getGen(), 0, unchosen.size() - 1).next();
            chosen[i] = list[unchosen[index]];
            unchosen.erase(unchosen.begin() + index);
        }
    }

    /// Randomly permute a list.  The entire list will be shuffled into a random order.  All permutations have equal probability.
    template<class T>
    void shuffle(vector<T>& list) const
    {
        //Standard swap shuffle algorithm
        for (szt i = list.size() - 1; i > 0; --i)
            std::swap(list[i], list[Discrete_d(getGen(), 0, i).next()]);
    }

    /// Generate a random unit direction
    Vec3 dir() const;

    /// Generate a random unit direction parallel to dir with an angular spread of dirVarMin to dirVarMax (radians)
    Vec3 dir(const Vec3& dir, Real dirVarMin, Real dirVarMax) const;

    /// Generate a 2D random unit direction
    Vec2 dir2d() const;

    /// Estimate the sample mean, returns a tuple of (mean, min, max)
    template<class Range>
    static tuple<Real,Real,Real> mean(const Range& samples)
    {
        auto res = reduce(samples, make_tuple(Real(0), Real_::inf, -Real_::inf),
            [](tuple<Real,Real,Real> a, Real e) { return make_tuple(get<0>(a) + e, Alge::min(get<1>(a), e), Alge::max(get<2>(a), e)); });
        szt n = end(samples) - begin(samples);
        get<0>(res) = n > 0 ? get<0>(res) / n : 0;
        return res;
    }

    /// Estimate the sample variance given the mean.  This is the unbiased estimator (mean over all possible samples is the population variance)
    template<class Range>
    static Real variance(const Range& samples, Real mean)
    {
        Real sumDev = reduce(samples, Real(0), [&](Real a, Real e) { return a + Alge::sqr(e - mean); });
        szt n = end(samples) - begin(samples);
        return n > 1 ? sumDev / (n-1) : 0;
    }

    /// Calculate the standard deviation given the variance
    static Real stdDev(Real variance)                                       { return Alge::sqrt(variance); }

    /// Calculate the standard error of the mean.  This is how well the sample mean appoximates the population mean. The larger the sample, the smaller the error.
    static Real stdErr(Real sampleSize, Real stddev)                        { return stddev / Alge::sqrt(sampleSize); }

    struct DistStats
    {
        szt     n;          ///< Sample size
        Real    mean;       ///< Sample mean
        Real    min;        ///< Minimum sample value
        Real    max;        ///< Maximum sample value
        Real    stdDev;     ///< Sample standard deviation
        Real    stdErr;     ///< Standard error of the mean (ie. standard deviation of the sample-mean estimate of the population mean)

        friend ostream& operator<<(ostream& os, const DistStats& val)
        {
            return os   << stringstream::indentInc << "{" << endl
                        << "N:       "  << val.n << endl
                        << "Mean:    "  << val.mean << endl
                        << "Min:     "  << val.min << endl
                        << "Max:     "  << val.max << endl
                        << "Std Dev: "  << val.stdDev << endl
                        << "Std Err: "  << val.stdErr
                        << stringstream::indentDec << endl  << "}";
        }
    };

    /// Calculate distribution statistics
    template<class Range>
    static DistStats stats(const Range& samples)
    {
        DistStats d;
        d.n = end(samples) - begin(samples);
        tie(d.mean, d.min, d.max) = mean(samples);
        d.stdDev = stdDev(variance(samples, d.mean));
        d.stdErr = stdErr(d.n, d.stdDev);
        return d;
    }

private:
    RandomGen* _gen;
};

typedef Random_<Real>       Random;
typedef Random_<Float>      Random_f;
typedef Random_<Double>     Random_d;

extern template class Random_<Float>;
extern template class Random_<Double>;

}

