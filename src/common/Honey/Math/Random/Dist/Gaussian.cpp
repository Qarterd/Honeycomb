// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/Gaussian.h"
#include "Honey/Math/Random/Random.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{


template<class Real>
Real Gaussian_<Real>::next() const
{
    // The standard Box-Muller transformation is performed,
    // producing two independent normally-distributed random deviates.
    // One is returned, the other is discarded.
    
    // choose a pair of uniformly distributed deviates, one for the
    // distance and one for the angle, and perform transformations
    Double dist = Alge_d::sqrt( -2 * Alge_d::log(Uniform::nextStd(getGen())) );
    Double angle = 2 * Double_::pi * Uniform::nextStd(getGen());
    
    // the first deviate, not used
    //Double deviate = dist*Trig_d::cos(angle);
    
    // return second deviate
    Double deviate = dist*Trig_d::sin(angle);
    return deviate*sigma + mu;
}

template<class Real>
Real Gaussian_<Real>::pdf(Real x) const
{
    static const Double sqrtTwoPi = Alge_d::sqrt(Double_::piTwo);
    Double xd = x, mud = mu, sigmad = sigma;
    return Alge_d::exp( -Alge_d::sqr(xd-mud) / (2*Alge_d::sqr(sigmad)) ) / (sigmad * sqrtTwoPi);
}

template<class Real>
Real Gaussian_<Real>::cdf(Real x) const
{
    static const Double a1 = 0.31938153, a2 = -0.356563782, a3 = 1.781477937, a4 = -1.821255978, a5 = 1.330274429;
    static const Double sqrtTwoPi = Alge_d::sqrt(Double_::piTwo);

    Double xz = Double(x-mu) / Double(sigma);
    Double L = Alge_d::abs(xz);
    Double K = 1 / (1 + 0.2316419 * L);
    Double w = 1 - 1 / sqrtTwoPi * Alge_d::exp(-L * L / 2) * (a1 * K + a2 * K * K + a3 * Alge_d::pow(K,3) + a4 * Alge_d::pow(K,4) + a5 * Alge_d::pow(K,5));

    if (xz < 0)
        w = 1 - w;
    return w;
}

//==============================================================================================================
/// Class to get inverse of gaussian
template<class Real>
class GaussianInv
{
    typedef typename Numeral<Real>::Real_ Real_;
    typedef Alge_<Real> Alge;
public:
    static Real calc(Real y0);
private:
    static Real polevl(Real x, const Real* coef, int N);
    static Real p1evl(Real x, const Real* coef, int N);

    static const Real P0[];
    static const Real Q0[];
    static const Real P1[];
    static const Real Q1[];
    static const Real P2[];
    static const Real Q2[];
};

/* approximation for 0 <= |y - 0.5| <= 3/8 */
template<class Real>
const Real GaussianInv<Real>::P0[5] = {
-5.99633501014107895267E1,
 9.80010754185999661536E1,
-5.66762857469070293439E1,
 1.39312609387279679503E1,
-1.23916583867381258016E0,
};
template<class Real>
const Real GaussianInv<Real>::Q0[8] = {
/* 1.00000000000000000000E0,*/
 1.95448858338141759834E0,
 4.67627912898881538453E0,
 8.63602421390890590575E1,
-2.25462687854119370527E2,
 2.00260212380060660359E2,
-8.20372256168333339912E1,
 1.59056225126211695515E1,
-1.18331621121330003142E0,
};

/* Approximation for interval z = sqrt(-2 log y ) between 2 and 8
  * i.e., y between exp(-2) = .135 and exp(-32) = 1.27e-14.
  */
template<class Real>
const Real GaussianInv<Real>::P1[9] = {
 4.05544892305962419923E0,
 3.15251094599893866154E1,
 5.71628192246421288162E1,
 4.40805073893200834700E1,
 1.46849561928858024014E1,
 2.18663306850790267539E0,
-1.40256079171354495875E-1,
-3.50424626827848203418E-2,
-8.57456785154685413611E-4,
};
template<class Real>
const Real GaussianInv<Real>::Q1[8] = {
/*  1.00000000000000000000E0,*/
 1.57799883256466749731E1,
 4.53907635128879210584E1,
 4.13172038254672030440E1,
 1.50425385692907503408E1,
 2.50464946208309415979E0,
-1.42182922854787788574E-1,
-3.80806407691578277194E-2,
-9.33259480895457427372E-4,
};

/* Approximation for interval z = sqrt(-2 log y ) between 8 and 64
  * i.e., y between exp(-32) = 1.27e-14 and exp(-2048) = 3.67e-890.
  */
template<class Real>
const Real GaussianInv<Real>::P2[9] = {
  3.23774891776946035970E0,
  6.91522889068984211695E0,
  3.93881025292474443415E0,
  1.33303460815807542389E0,
  2.01485389549179081538E-1,
  1.23716634817820021358E-2,
  3.01581553508235416007E-4,
  2.65806974686737550832E-6,
  6.23974539184983293730E-9,
};
template<class Real>
const Real GaussianInv<Real>::Q2[8] = {
/*  1.00000000000000000000E0,*/
  6.02427039364742014255E0,
  3.67983563856160859403E0,
  1.37702099489081330271E0,
  2.16236993594496635890E-1,
  1.34204006088543189037E-2,
  3.28014464682127739104E-4,
  2.89247864745380683936E-6,
  6.79019408009981274425E-9,
};

///Evaluate polynomial
/**
  * Evaluates polynomial of degree N:
  *
  *                    2          N
  * y  =  C  + C x + C x  +...+ C x
  *       0    1     2          N
  *
  * Coefficients are stored in reverse order:
  *
  * coef[0] = C  , ..., coef[N] = C  .
  *           N                   0
  *
  * The function p1evl() assumes that coef[N] = 1.0 and is
  * omitted from the array.  Its calling arguments are
  * otherwise the same as polev().
  */

template<class Real>
Real GaussianInv<Real>::polevl(Real x, const Real* coef, int N)
{
    const Real* p = coef;
    Real ans = *p++;
    int i = N;

    do
    {
        ans = ans * x + *p++;
    } while( --i );

    return ans;
}
                                          
template<class Real>
Real GaussianInv<Real>::p1evl(Real x, const Real* coef, int N)
{
    const Real* p = coef;
    Real ans = x + *p++;
    int i = N-1;

    do
    {
        ans = ans * x + *p++;
    } while( --i );

    return ans;
}

/// Inverse of Normal distribution function
/**
  * Returns the argument, x, for which the area under the
  * Gaussian probability density function (integrated from
  * minus infinity to x) is equal to y.
  *
  * For small arguments 0 < y < exp(-2), the program computes
  * z = sqrt( -2.0 * log(y) );  then the approximation is
  * x = z - log(z)/z  - (1/z) P(1/z) / Q(1/z).
  * There are two rational functions P/Q, one for 0 < y < exp(-32)
  * and the other for y up to exp(-2).  For larger arguments,
  * w = y - 0.5, and  x/sqrt(2pi) = w + w**3 R(w**2)/S(w**2)).
  */
template<class Real>
Real GaussianInv<Real>::calc(Real y0)
{
    if (y0 <= 0)
        return -Real_::max;

    if (y0 >= 1)
        return Real_::max;

    int code = 1;
    Real y = y0;
    if (y > 1 - 0.13533528323661269189) /* 0.135... = exp(-2) */
    {
        y = 1 - y;
        code = 0;
    }

    if (y > 0.13533528323661269189)
    {
        y = y - 0.5;
        Real y2 = y * y;
        Real x = y + y * (y2 * polevl(y2, P0, 4) / p1evl(y2, Q0, 8));
        static const Real sqrtTwoPi = Alge::sqrt(Real_::piTwo);
        x = x * sqrtTwoPi; 
        return x;
    }

    Real x = Alge::sqrt( -2 * Alge::log(y) );
    Real x0 = x - Alge::log(x)/x;

    Real z = 1/x;
    Real x1;
    if(x < 8) /* y > exp(-32) = 1.2664165549e-14 */
        x1 = z * polevl(z, P1, 8) / p1evl(z, Q1, 8);
    else
        x1 = z * polevl(z, P2, 8) / p1evl(z, Q2, 8);
    x = x0 - x1;
    if(code != 0)
        x = -x;
    return x;
}
//==============================================================================================================

template<class Real>
Real Gaussian_<Real>::cdfInv(Real P) const
{
    return Double(mu) + Double(sigma) * GaussianInv<Double>::calc(Double(P));
}

template class Gaussian_<Float>;
template class Gaussian_<Double>;

}
