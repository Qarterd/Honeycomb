// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Random.h"

namespace honey
{

/// Monte Carlo (random-based) method to approximate the integral of a function over any number of dimensions.
/**
  * Example: Find the mass of a sphere with radius 2 and density 0.5
  *
  *     //Functor takes a 3D coordinate and returns a 1D density sample
  *     typedef Vegas<3, 1> Vegas;
  *     struct Func { Vegas::VecRes operator()(const Vegas::Vec& coord)   { return coord.length() <= 2 ? 0.5 : 0; }  };
  *
  *     //Integrate over 3D bounding box [-2, 2]
  *     Real mass = Vegas(Func(), Chacha(), Vegas::Vec(-2), Vegas::Vec(2)).integrate();
  *     //mass =~ 16.7
  *
  * \tparam Dim         The dimension of the function input, number of variables that func operates on
  * \tparam DimRes      The dimension of the function result, each dimension of the result will be averaged separately over the integration region
  * \tparam BinCount    Tunable param, each dimension of the input (Dim) will be divided into BinCount separate bins.  Higher bin counts provide more accuracy.
  *                     At the default bin count of 100 a Vegas instance can fit on the stack, providing better performance.
  *
  * Algorithm from: "VEGAS: An Adaptive Multi-dimensional Integration Program", G.P. Lepage, 1980. \n
  * Code adapted from C implementation by Richard Kreckel.  
  */
template<int Dim = 1, int DimRes = 1, class Real__ = Real, int BinCount = 100>
class Vegas
{
public:
    typedef Real__                          Real;
private:
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    typedef Uniform_<Real>                  Uniform;

public:
    static const int dim                    = Dim;
    static const int dimRes                 = DimRes;
    typedef Vec<dimRes,Real>                VecRes;
    typedef Vec<dim,Real>                   Vec;
    typedef function<VecRes (const Vec&)>   Func;

    /// Constructor, set up constants for all integration calls
    /**
      * \param func         Function to integrate
      * \param gen          Random generator
      * \param lower        Lower bound of the region over which to integrate (has dimension Dim)
      * \param upper        Upper bound of the region over which to integrate (has dimension Dim)
      * \param sampleCount  Number of functor calls per iteration.  If results are not reliable then increase the sample count (check stdDev() or chiSqr())
      * \param warmUp       Tunable param, percentage of samples (range >= 0) to use for warming up the grid. These are extra samples, results are discarded.
      * \param iterCount    Tunable param, number of iterations, functor will be called roughly sampleCount times per iteration.
      * \param alpha        Tunable param, represents stiffness of the grid rebinning algorithm. Range is usually [1,2], no rebinning will occur at 0.
      */
    Vegas(const Func& func, RandomGen& gen, const Vec& lower, const Vec& upper, int sampleCount = 1000, Real warmUp = 0.1, int iterCount = 5, Real alpha = 1.5) :

        func(func), gen(gen), lower(lower), upper(upper), sampleCount(sampleCount), warmUp(Alge::max(warmUp,0)), iterCount(iterCount), alpha(alpha),
        tgral(0), chi2a(0), sd(0), _progress(0), sampleTotal(0) {}

    ~Vegas() {}

    /// Perform integral calculation. The calculation can be split up over multiple calls.
    /**
      * \param progressDelta    Percentage of progress to complete in this call. Range [0,1]
      * \return                 Result of integration so far.  Accuracy increases with progress.
      */
    const VecRes& integrate(Real progressDelta = 1);

    /// Current progress of calculation, from 0 (start) to 1 (complete).
    Real progress() const                                                           { return _progress; }
    /// Get current result of integration (same value returned by Integrate)
    const VecRes& result() const                                                    { return tgral; }
    /// Get chi-square statistic for integral.  A value that differs significantly from 1 (eg. diff > 0.5) indicates an unreliable result and more samples or iterations are required.
    const VecRes& chiSqr() const                                                    { return chi2a; }
    /// Estimate of standard deviation of integral result.  Indicative of +- error range in result.
    const VecRes& stdDev() const                                                    { return sd; }

private:
    void init(int init, int sampleCount);
    void rebin(Real rc, int nd, Real r[], Real xin[], Real xi[]);
    void integrate_priv(int samplesMax);

    // Input
    Func func;
    RandomGen& gen;
    Vec lower;
    Vec upper;
    int sampleCount;
    Real warmUp;
    int iterCount;
    Real alpha;

    // Output
    VecRes tgral;
    VecRes chi2a;
    VecRes sd;

    // Progress
    Real _progress;
    int sampleTotal;
    int iterCur;
    int npgCur;

    // Init 0 and 1
    int ndo;
    int ittot;                  // iter count across init > 1
    int mds;                    // sampling mode

    // Init 2
    int nd;                     // slices in grid (c.f. BinCount)
    int ng;  
    int npg;                    // number of calls within bin
    Real calls;                 // real total number of calls to fxn
    Real dv2g;
    Real dxg;
    Real xnd;
    Real xJac;                  // Jacobian of integration
    Real dx[dim];               // width of integration region

    // Integrate locals
    // accumulator for stuff at end of iteration...
    struct iterAccu
    {
        Real Wgt;               // weight
        Real sWgt;              // cumulative sum for weights
        Real sChi;              // cumulative sum for chi^2
        Real sInt;              // cumulative sum for integral
    };                          
    iterAccu Ai[dimRes];        // ...one for each integrand

    Real d[BinCount][dim];
    Real di[BinCount][dim];     // delta i
    Real r[BinCount];
    Real xi[dim][BinCount];
    Real xin[BinCount];         // aux. variable for rebinning

    int ia[dim];
    int kg[dim];

    // accumulator over bins / hypercubes...
    struct binAccu
    {
        Real ti;                // sum for f over bins
        Real tsi;               // sum for variances over bins
    };                 
    binAccu Ab[dimRes];         // ...one for each integrand

    // accumulator over points x within bins...
    struct pointAccu
    {
        Real f2;                // f squared
        Real fb;                // sum for f within bi
        Real f2b;               // sum for f2 within bin
        int npg;                // number of calls within bin f != 0
    };               
    pointAccu Ax[dimRes];       // ...one for each integrand
};

template<int Dim, int DimRes, class Real, int BinCount>
const typename Vegas<Dim,DimRes,Real,BinCount>::VecRes&
    Vegas<Dim,DimRes,Real,BinCount>::integrate(Real progressDelta)
{
    int sampleWarmUp = warmUp*sampleCount;
    if (_progress == 0)
    {
        //First run, init with warm up or main
        if (sampleWarmUp > 0)
            init(0, sampleWarmUp);
        else
            init(0, sampleCount);
    }
    
    _progress = Alge::min(_progress + progressDelta, 1);
    int sampleAcc = _progress*(sampleWarmUp+sampleCount) - sampleTotal;
    sampleTotal += sampleAcc;

    if (sampleTotal - sampleAcc < sampleWarmUp)
    {
        //Warm up grid
        integrate_priv(sampleTotal < sampleWarmUp ? sampleAcc : -1);
        if (sampleTotal < sampleWarmUp)
            return result();
        //Done warm up, discard results and init main
        sampleAcc = sampleTotal - sampleWarmUp;
        init(1, sampleCount);
    }

    //Piecewise sample count is not accurate, so -1 means run until completion
    integrate_priv(_progress < 1 ? sampleAcc : -1);

    if (iterCur == iterCount)
        //Calculation is complete
        _progress = 1;

    return result();
}

template<int Dim, int DimRes, class Real, int BinCount>
void Vegas<Dim,DimRes,Real,BinCount>::init(int init, int sampleCount)
{
    iterCur = 0;
    npgCur = -2;

    if (init <= 0)      //entry for cold start
    {
        mds = 1;        //1 == use stratified sampling
        ndo = 1;
        for (int j=0; j<dim; ++j) xi[j][0] = 1;
    }

    if (init <= 1)      //inherit the previous grid
    {
        for (int j=0; j<dimRes; ++j)
        {
            Ai[j].sInt = 0;
            Ai[j].sWgt = 0;
            Ai[j].sChi = 0;
        }
        ittot = 1;
    }

    if (init <= 2)      //inherit grid and results
    {
        nd = BinCount;
        ng = 1;
        if (mds)
        {
            ng = Alge::pow(Real(sampleCount)/2+0.25,1/Real(dim));
            mds = 1;
            if ((2*ng-BinCount) >= 0)
            {
                mds = -1;
                npg = ng/BinCount+1;
                nd = ng/npg;
                ng = npg*nd;
            }
        }
        if (ng <= 0)
        {
            //No samples
            iterCur = iterCount;
            return;
        }
        int k = 1;
        for (int i=0; i<dim; ++i) k *= ng;
        npg = sampleCount/k > 2 ? sampleCount/k : 2;
        calls = npg * k;
        dxg = 1/Real(ng);
        dv2g = 1;
        for (int i=0; i<dim; ++i) dv2g *= dxg;
        dv2g = calls*calls*dv2g*dv2g/npg/npg/(npg-1.0);
        xnd = nd;
        dxg *= xnd;
        xJac = 1/Real(calls);
        for (int j=0; j<dim; ++j)
        {
            dx[j] = upper[j]-lower[j];
            xJac *= dx[j];
        }
        if (nd != ndo)
        {
            for (int i=0; i<(nd>ndo?nd:ndo); ++i) r[i] = 1;
            for (int j=0; j<dim; ++j) rebin(ndo/xnd,nd,r,xin,xi[j]);
            ndo = nd;
        }
    }
}

template<int Dim, int DimRes, class Real, int BinCount>
void Vegas<Dim,DimRes,Real,BinCount>::integrate_priv(int samplesMax)
{
    int samples = 0;
    samplesMax *= iterCount;

    for (; iterCur < iterCount; ++iterCur, ++ittot)
    {
        if (npgCur == -2)
        {
            //Init loop
            npgCur = -1;
            for (int j=0; j<dimRes; ++j) Ab[j].ti = Ab[j].tsi = 0;
            for (int j=0; j<dim; ++j)
            {
                kg[j] = 1;
                for (int i=0; i<nd; ++i) d[i][j] = di[i][j] = 0;
            }
        }
        for (;;)
        {
            if (npgCur == -1)
            {
                //Init loop
                npgCur = 0;
                for (int j=0; j<dimRes; ++j)
                {
                    Ax[j].fb = 0;
                    Ax[j].f2b = 0;
                    Ax[j].npg = 0;
                }
            }
            for (int k=npgCur; k<npg; ++k, ++samples)
            {
                if (samplesMax >= 0 && samples >= samplesMax)
                {
                    npgCur = k;
                    return;
                }

                Real wgt = xJac;
                Vec x;
                for (int j=0; j<dim; ++j)
                {
                    Real xrand = Uniform::nextStd(gen);
                    Real xn = (kg[j]-xrand)*dxg+1;
                    ia[j] = (xn<BinCount) ? xn : BinCount;
                    ia[j] = (ia[j]>1) ? ia[j] : 1;
                    Real xo, rc;
                    if (ia[j] > 1)
                    {
                        xo = xi[j][ia[j]-1]-xi[j][ia[j]-2];
                        rc = xi[j][ia[j]-2]+(xn-ia[j])*xo;
                    }
                    else
                    {
                        xo = xi[j][ia[j]-1];
                        rc = (xn-ia[j])*xo;
                    }
                    x[j] = lower[j]+rc*dx[j];
                    wgt *= xo*xnd;
                }
                VecRes f = func(const_cast<const Vec&>(x));   // call integrand at point x
                for (int j=0; j<dimRes; ++j)
                {
                    if (f[j] != 0) ++Ax[j].npg;
                    f[j] *= wgt;
                    Ax[j].f2 = f[j]*f[j];
                    Ax[j].fb += f[j];
                    Ax[j].f2b += Ax[j].f2;
                }
                for (int j=0; j<dim; ++j)
                {
                    di[ia[j]-1][j] += f[0];
                    if (mds >= 0) d[ia[j]-1][j] += Ax[0].f2;
                }
            } // end of loop within hypercube
            npgCur = -1;

            for (int j=0; j<dimRes; ++j)
            {
                Ax[j].f2b = Alge::sqrt(Ax[j].f2b*Ax[j].npg);
                Ax[j].f2b = (Ax[j].f2b-Ax[j].fb)*(Ax[j].f2b+Ax[j].fb);
                if (Ax[j].f2b <= 0) Ax[j].f2b = Real_::smallest;
                Ab[j].ti += Ax[j].fb;
                Ab[j].tsi += Ax[j].f2b;
            }
            if (mds < 0)
            {
                for (int j=0; j<dim; ++j) d[ia[j]-1][j] += Ax[0].f2b;
            }
            int k;
            for (k=dim-1; k>=0; --k)
            {
                kg[k] %= ng;
                if (++kg[k] != 1) break;
            }
            if (k < 0) break;
        }  // end of loop over hypercubes
        npgCur = -2;

        for (int j=0; j<dimRes; ++j)
        {
            Ab[j].tsi *= dv2g;
            Ai[j].Wgt = 1/Ab[j].tsi;
            Ai[j].sInt += Ai[j].Wgt*Ab[j].ti;
            Ai[j].sChi += Ai[j].Wgt*Ab[j].ti*Ab[j].ti;
            Ai[j].sWgt += Ai[j].Wgt;
            tgral[j] = Ai[j].sInt/Ai[j].sWgt;
            chi2a[j] = (Ai[j].sChi-Ai[j].sInt*tgral[j])/(ittot-0.9999);
            if (chi2a[j] < 0) chi2a[j] = 0;
            sd[j] = Alge::sqrt(1/Ai[j].sWgt);
            Ab[j].tsi = Alge::sqrt(Ab[j].tsi);
        }

        Real dt[dim];
        for (int j=0; j<dim; ++j)
        {
            Real xo = d[0][j];
            Real xn = d[1][j];
            d[0][j] = (xo+xn)/2;
            dt[j] = d[0][j];
            for (int i=1; i<nd-1; ++i)
            {
                Real rc = xo+xn;
                xo = xn;
                xn = d[i+1][j];
                d[i][j] = (rc+xn)/3;
                dt[j] += d[i][j];
            }
            d[nd-1][j] = (xo+xn)/2;
            dt[j] += d[nd-1][j];
        }
        for (int j=0; j<dim; ++j)
        {
            Real rc = 0;
            for (int i=0; i<nd; ++i)
            {
                if (d[i][j] < Real_::smallest) d[i][j] = Real_::smallest;
                r[i] =  Alge::pow((1.0-d[i][j]/dt[j])/
                        (Alge::log(dt[j])-Alge::log(d[i][j])),alpha);
                rc += r[i];
            }
            rebin(rc/xnd,nd,r,xin,xi[j]);
        }
    }
}

template<int Dim, int DimRes, class Real, int BinCount>
void Vegas<Dim, DimRes, Real, BinCount>::rebin(Real rc, int nd, Real r[], Real xin[], Real xi[])
{
    int i;
    int k = 0;
    Real dr = 0;
    Real xn = 0;
    Real xo = 0;

    for (i=0; i<nd-1; i++)
    {
        while (rc > dr)
            dr += r[k++];
        if (k > 1) xo = xi[k-2];
        xn = xi[k-1];
        dr -= rc;
        xin[i] = xn-(xn-xo)*dr/r[k-1];
    }
    for (i=0; i<nd-1; i++) xi[i] = xin[i];
    xi[nd-1] = 1;
}

}

