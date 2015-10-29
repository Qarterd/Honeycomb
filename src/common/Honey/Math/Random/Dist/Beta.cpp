// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/Dist/Beta.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{

template<class Real>
Real Beta_<Real>::next() const
{
    Double x1 = Gamma(getGen(), Double(a), 1).next();
    Double x2 = Gamma(getGen(), Double(b), 1).next();
    return x1 / (x1 + x2);
}

template<class Real>
Real Beta_<Real>::pdf(Real x) const
{
    if (x < 0 || x > 1)
        return 0;
    else
    {
        Double xd = x, ad = a, bd = b;
        Double gab = GammaFunc::gammaLn(ad + bd);
        Double ga = GammaFunc::gammaLn(ad);
        Double gb = GammaFunc::gammaLn(bd);
      
        if (x == 0 || x == 1)
            return Alge_d::exp(gab - ga - gb) * Alge_d::pow(xd, ad-1) * Alge_d::pow(1-xd, bd-1);
        else
            return Alge_d::exp(gab - ga - gb + Alge_d::log(xd)*(ad-1) + Alge_d::log1p(-xd) * (bd-1));
    }
}

template<class Real>
Real Beta_<Real>::cdf(Real x) const
{
    return BetaInc::calc(Double(x), Double(a), Double(b));
}

template<class Real>
Real Beta_<Real>::cdfInv(Real P) const
{
    return BetaInc::calcInv(Double(P), Double(a), Double(b));
}

template class Beta_<Float>;
template class Beta_<Double>;



//==============================================================================================================

template<class Real>
Real BetaInc<Real>::calc(Real x, Real a, Real b)
{
    if (x < 0)
        return 0;

    Real fx2;

    /*
    debug_print(sout()
        << "  BetaIn computes the incomplete Beta function.\n"
        << "  Compare to tabulated values.\n"
        << "\n"
        << "           A           B           X          FX                        FX2\n"
        << "                                              (Tabulated)               (BetaIn)            DIFF\n"
        << "\n"
        );
    
    int n_data = 0;
    
    for (;;)
    {
        Real fx;
        values(n_data, a, b, x, fx);
        if (n_data == 0) break;
    */
        int fault;
        fx2 = betaIn(x, a, b, GammaFunc::gammaLn(a) + GammaFunc::gammaLn(b) - GammaFunc::gammaLn(a + b), fault);
    /*
        debug_print(sout()
            << "  " << setprecision(4) << std::setw(10) << a
            << "  " << setprecision(4) << std::setw(10) << b
            << "  " << setprecision(4) << std::setw(10) << x
            << "  " << setprecision(16) << std::setw(24) << fx
            << "  " << setprecision(16) << std::setw(24) << fx2
            << "  " << setprecision(4) << std::setw(10) << Alge::abs(fx - fx2) << "\n"
            );
    }
    */

    return fx2;
}

/// Returns some values of the incomplete Beta function.
/**
  * The incomplete Beta function may be written
  *
  * BETA_INC(A,B,X) = Integral(0..X){ T^{A-1} * (1-T)^{B-1} dT }
  *                 / Integral(0..1){ T^{A-1} * (1-T)^{B-1} dT }
  *
  * Thus,
  *
  * BETA_INC(A,B,0.0) = 0.0;
  * BETA_INC(A,B,1.0) = 1.0
  *
  * The incomplete Beta function is also sometimes called the "modified" Beta function, or the "normalized" Beta function
  * or the Beta CDF (cumulative density function.
  *
  * \param n_data   The user sets n_data to 0 before the first call.  On each call, the routine increments n_data by 1, and
  *                 returns the corresponding data; when there is no more data, the output value of n_data will be 0 again.
  *
  * \param a, b     The parameters of the function.
  * \param x        The argument of the function.
  * \param fx       The value of the function.
  */
template<class Real>
void BetaInc<Real>::values(int& n_data, Real& a, Real& b, Real& x, Real& fx)
{
    # define N_MAX 42

    const Real a_vec[N_MAX] = {
        0.5E+00,    0.5E+00,   0.5E+00,  1.0E+00,    1.0E+00,   1.0E+00,   1.0E+00,   1.0E+00,   2.0E+00,   2.0E+00,
        2.0E+00,    2.0E+00,   2.0E+00,  2.0E+00,    2.0E+00,   2.0E+00,   2.0E+00,   5.5E+00,  10.0E+00,  10.0E+00,
        10.0E+00,   10.0E+00,  20.0E+00, 20.0E+00,   20.0E+00,  20.0E+00,  20.0E+00,  30.0E+00,  30.0E+00,  40.0E+00,
        0.1E+01,    0.1E+01,   0.1E+01,  0.1E+01,    0.1E+01,   0.1E+01,   0.1E+01,   0.1E+01,   0.2E+01,   0.3E+01, 
        0.4E+01,    0.5E+01 };

    const Real b_vec[N_MAX] = { 
        0.5E+00,  0.5E+00,  0.5E+00,  0.5E+00,  0.5E+00,  0.5E+00,  0.5E+00,  1.0E+00,  2.0E+00,  2.0E+00,  
        2.0E+00,  2.0E+00,  2.0E+00,  2.0E+00,  2.0E+00,  2.0E+00,  2.0E+00,  5.0E+00,  0.5E+00,  5.0E+00,  
        5.0E+00, 10.0E+00,  5.0E+00, 10.0E+00, 10.0E+00, 20.0E+00, 20.0E+00, 10.0E+00, 10.0E+00, 20.0E+00,
        0.5E+00,  0.5E+00,  0.5E+00,  0.5E+00,  0.2E+01,  0.3E+01,  0.4E+01,  0.5E+01,  0.2E+01,  0.2E+01, 
        0.2E+01,  0.2E+01 };

    const Double fx_vec[N_MAX] = { 
        0.6376856085851985E-01,  0.2048327646991335E+00,  0.1000000000000000E+01,  0.0000000000000000E+00,  0.5012562893380045E-02,  
        0.5131670194948620E-01,  0.2928932188134525E+00,  0.5000000000000000E+00,  0.2800000000000000E-01,  0.1040000000000000E+00,  
        0.2160000000000000E+00,  0.3520000000000000E+00,  0.5000000000000000E+00,  0.6480000000000000E+00,  0.7840000000000000E+00,  
        0.8960000000000000E+00,  0.9720000000000000E+00,  0.4361908850559777E+00,  0.1516409096347099E+00,  0.8978271484375000E-01,  
        0.1000000000000000E+01,  0.5000000000000000E+00,  0.4598773297575791E+00,  0.2146816102371739E+00,  0.9507364826957875E+00,  
        0.5000000000000000E+00,  0.8979413687105918E+00,  0.2241297491808366E+00,  0.7586405487192086E+00,  0.7001783247477069E+00,
        0.5131670194948620E-01,  0.1055728090000841E+00,  0.1633399734659245E+00,  0.2254033307585166E+00,  0.3600000000000000E+00, 
        0.4880000000000000E+00,  0.5904000000000000E+00,  0.6723200000000000E+00,  0.2160000000000000E+00,  0.8370000000000000E-01, 
        0.3078000000000000E-01,  0.1093500000000000E-01 };

    const Double x_vec[N_MAX] = { 
        0.01E+00,  0.10E+00,  1.00E+00,  0.00E+00,  0.01E+00,  0.10E+00,  0.50E+00,  0.50E+00,  0.10E+00,  0.20E+00,
        0.30E+00,  0.40E+00,  0.50E+00,  0.60E+00,  0.70E+00,  0.80E+00,  0.90E+00,  0.50E+00,  0.90E+00,  0.50E+00,
        1.00E+00,  0.50E+00,  0.80E+00,  0.60E+00,  0.80E+00,  0.50E+00,  0.60E+00,  0.70E+00,  0.80E+00,  0.70E+00,
        0.10E+00,  0.20E+00,  0.30E+00,  0.40E+00,  0.20E+00,  0.20E+00,  0.20E+00,  0.20E+00,  0.30E+00,  0.30E+00,
        0.30E+00,  0.30E+00 };

    if (n_data < 0)
        n_data = 0;

    if (n_data < N_MAX)
    {
        a = a_vec[n_data];
        b = b_vec[n_data];
        x = x_vec[n_data];
        fx = fx_vec[n_data];
        n_data++;
    }
    else
    {
        n_data = 0;
        a = 0;
        b = 0;
        x = 0;
        fx = 0;
    }

    # undef N_MAX
    
    return;
}

/// Computes the incomplete Beta function ratio.
/**
  * \param x        The argument, between 0 and 1.
  * \param p,q      The parameters, which must be positive.
  * \param beta     The logarithm of the complete beta function.
  * \param fault    Error flag. 0, no error. nonzero, an error occurred.
  * \return         The value of the incomplete Beta function ratio.
  */
template<class Real>
Real BetaInc<Real>::betaIn(Real x, Real p, Real q, Real beta, int& fault)
{
    fault = 0;
    Real value = x;

    //  Check the input arguments.
    if (p <= 0 || q <= 0)
    {
        fault = 1;
        return value;
    }

    if (x < 0 || 1 < x)
    {
        fault = 2;
        return value;
    }

    //  Special cases.
    if (x == 0 || x == 1)
        return value;

    //  Change tail if necessary and determine S.
    Real psq = p + q;
    Real cx = 1 - x;
    Real xx;
    Real pp;
    Real qq;
    bool indx;
    if (p < psq * x)
    {
        xx = cx;
        cx = x;
        pp = q;
        qq = p;
        indx = true;
    }
    else
    {
        xx = x;
        pp = p;
        qq = q;
        indx = false;
    }

    Real term = 1;
    Real ai = 1;
    value = 1;
    int ns = (int)(qq + cx * psq);

    //  Use the Soper reduction formula.
    Real rx = xx / cx;
    Real temp = qq - ai;
    if (ns == 0)
        rx = xx;

    static const int iterMax = 1000;
    static const Real errtol = 0.1E-14;
    for (int i = 0;; ++i)
    {
        term = term * temp * rx / ( pp + ai );
        value = value + term;
        temp = Alge::abs(term);
        
        if ((temp <= errtol && temp <= errtol * value) || i > iterMax)
        {
            value = value * Alge::exp(pp * Alge::log(xx) + (qq - 1) * Alge::log(cx) - beta) / pp;
            if (indx)
                value = 1 - value;
            break;
        }

        ai = ai + 1;
        ns = ns - 1;

        if (0 <= ns)
        {
            temp = qq - ai;
            if (ns == 0)
                rx = xx;
        }
        else
        {
            temp = psq;
            psq = psq + 1;
        }
    }

    return value;
}


/// Inverse of imcomplete beta integral
/**
  * Given y, the function finds x such that
  * incbet( a, b, x ) = y
  *
  * The routine performs interval halving or Newton iterations to find the
  * root of incbet(a,b,x) - y = 0
  */
template<class Real>
Real BetaInc<Real>::calcInv(Real yy0, Real aa, Real bb)
{
    if (yy0 <= 0)
        return 0;
    if (yy0 >= 1)
        return 1;

    int i = 0;
    Real x0 = 0;
    Real yl = 0;
    Real x1 = 1;
    Real yh = 1;
    int nflg = 0;
    Real a, b, x, y, y0, dithresh, d, yp, xt, di, lgm;
    int rflg, dir;

    if (aa <= 1 || bb <= 1)
    {
        dithresh = 1.0e-6;
        rflg = 0;
        a = aa;
        b = bb;
        y0 = yy0;
        x = a/(a+b);
        y = calc(x, a, b);
        goto ihalve;
    }
    else
    {
        dithresh = 1.0e-4;
    }
    /* approximation to inverse function */

    yp = -Gaussian().cdfInv(yy0);

    if (yy0 > 0.5)
    {
        rflg = 1;
        a = bb;
        b = aa;
        y0 = 1.0 - yy0;
        yp = -yp;
    }
    else
    {
        rflg = 0;
        a = aa;
        b = bb;
        y0 = yy0;
    }

    lgm = (yp * yp - 3)/6;
    x = 2 / ( 1/(2*a-1) + 1/(2*b-1) );
    d = yp * Alge::sqrt(x + lgm) / x
        - ( 1/(2*b-1) - 1/(2*a-1) )
        * (lgm + 5./6 - 2/(3*x));
    d = 2 * d;
    if (d < Alge::logMin)
    {
        x = 1;
        goto under;
    }
    x = a/( a + b * Alge::exp(d) );
    y = calc(x, a, b);
    yp = (y - y0)/y0;
    if (Alge::abs(yp) < 0.2)
        goto newt;

    /* Resort to interval halving if not close enough. */
    ihalve:

    dir = 0;
    di = 0.5;
    for (i = 0; i < 100; ++i)
    {
        if( i != 0 )
        {
            x = x0  +  di * (x1 - x0);
            if (x == 1)
                x = 1 - Real_::epsilon;
            if (x == 0)
            {
                di = 0.5;
                x = x0 + di * (x1 - x0);
                if (x == 0)
                    goto under;
            }
            y = calc(x, a, b);
            yp = (x1 - x0)/(x1 + x0);
            if (Alge::abs(yp) < dithresh)
                goto newt;
            yp = (y-y0)/y0;
            if (Alge::abs(yp) < dithresh)
                goto newt;
        }
        if (y < y0)
        {
            x0 = x;
            yl = y;
            if( dir < 0 )
            {
                dir = 0;
                di = 0.5;
            }
            else if (dir > 3)
                di = 1 - (1 - di) * (1 - di);
            else if (dir > 1)
                di = 0.5 * di + 0.5; 
            else
                di = (y0 - y)/(yh - yl);
            dir += 1;
            if (x0 > 0.75)
            {
                if (rflg == 1)
                {
                    rflg = 0;
                    a = aa;
                    b = bb;
                    y0 = yy0;
                }
                else
                {
                    rflg = 1;
                    a = bb;
                    b = aa;
                    y0 = 1 - yy0;
                }
                x = 1 - x;
                y = calc(x, a, b);
                x0 = 0;
                yl = 0;
                x1 = 1;
                yh = 1;
                goto ihalve;
            }
        }
        else
        {
            x1 = x;
            if (rflg == 1 && x1 < Real_::epsilon)
            {
                x = 0;
                goto done;
            }
            yh = y;
            if (dir > 0)
            {
                dir = 0;
                di = 0.5;
            }
            else if (dir < -3)
                di = di * di;
            else if (dir < -1)
                di = 0.5 * di;
            else
                di = (y - y0)/(yh - yl);
            dir -= 1;
        }
    }

    if (x0 >= 1)
    {
        x = 1 - Real_::epsilon;
        goto done;
    }
    if (x <= 0)
    {
    under:
        x = 0;
        goto done;
    }

    newt:

    if( nflg )
        goto done;
    nflg = 1;
    lgm = GammaFunc::gammaLn(a+b) - GammaFunc::gammaLn(a) - GammaFunc::gammaLn(b);

    for (i = 0; i < 8; ++i)
    {
        /* Compute the function at this point. */
        if (i != 0)
            y = calc(x,a,b);
        if (y < yl)
        {
            x = x0;
            y = yl;
        }
        else if (y > yh)
        {
            x = x1;
            y = yh;
        }
        else if (y < y0)
        {
            x0 = x;
            yl = y;
        }
        else
        {
            x1 = x;
            yh = y;
        }
        if (x == 1 || x == 0)
            break;
        /* Compute the derivative of the function at this point. */
        d = (a - 1) * Alge::log(x) + (b - 1) * Alge::log(1-x) + lgm;
        if (d < Alge::logMin)
            goto done;
        if (d > Alge::logMax)
            break;
        d = Alge::exp(d);
        /* Compute the step to the next approximation of x. */
        d = (y - y0)/d;
        xt = x - d;
        if (xt <= x0)
        {
            y = (x - x0) / (x1 - x0);
            xt = x0 + 0.5 * y * (x - x0);
            if (xt <= 0)
                break;
        }
        if (xt >= x1)
        {
            y = (x1 - x) / (x1 - x0);
            xt = x1 - 0.5 * y * (x1 - x);
            if (xt >= 1)
                break;
        }
        x = xt;
        if (Alge::abs(d/x) < 128 * Real_::epsilon)
            goto done;
    }
    /* Did not converge.  */
    dithresh = 256 * Real_::epsilon;
    goto ihalve;

    done:

    if( rflg )
    {
        if (x <= Real_::epsilon)
            x = 1 - Real_::epsilon;
        else
            x = 1 - x;
    }
    return x;
}


template class BetaInc<Float>;
template class BetaInc<Double>;

}
