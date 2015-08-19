// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Alge/Alge.h"
#include "Honey/Math/NumAnalysis/Interp.h"

namespace honey
{

template<class Real> const Real Alge_<Real>::logMin = log(numeral<Real>().smallest());
template<class Real> const Real Alge_<Real>::logMax = log(numeral<Real>().max());

template<class Real>
Real Alge_<Real>::log1p(const Real x)
{
    if(x < -1)
        return Real_::nan;

    if(x == -1)
        return -Real_::inf;

    Real a = abs(x);
    if(a > 0.5)
        return log(1 + x);
    if(a < Real_::epsilon)
      return x;

    static const Real P[] = {
        0.15141069795941984e-16L,
        0.35495104378055055e-15L,
        0.33333333333332835L,
        0.99249063543365859L,
        1.1143969784156509L,
        0.58052937949269651L,
        0.13703234928513215L,
        0.011294864812099712L
        };

    static const Real Q[] = {
        1L,
        3.7274719063011499L,
        5.5387948649720334L,
        4.159201143419005L,
        1.6423855110312755L,
        0.31706251443180914L,
        0.022665554431410243L,
        -0.29252538135177773e-5L
        };

    Real result = 1 - x / 2 +
                    (((((((P[7] * x + P[6]) * x + P[5]) * x + P[4]) * x + P[3]) * x + P[2]) * x + P[1]) * x + P[0]) /
                    (((((((Q[7] * x + Q[6]) * x + Q[5]) * x + Q[4]) * x + Q[3]) * x + Q[2]) * x + Q[1]) * x + Q[0]);

    return result*x;
}

template<class Real>
Real Alge_<Real>::expm1(const Real x)
{
    Real a = abs(x);
    if(a > 0.5)
    {
        if(a >= logMax)
        {
            if(x > 0)
                return Real_::inf;
            return -1;
        }
        return exp(x) - 1;
    }
    if(a < Real_::epsilon)
        return x;

    static const Real Y = 0.10281276702880859e1L;
    static const Real n[] = { -0.28127670288085937e-1L, 0.51278186299064534e0L, -0.6310029069350198e-1L, 0.11638457975729296e-1L, -0.52143390687521003e-3L, 0.21491399776965688e-4L };
    static const Real d[] = { 1, -0.45442309511354755e0L, 0.90850389570911714e-1L, -0.10088963629815502e-1L, 0.63003407478692265e-3L, -0.17976570003654402e-4L };

    Real result = x * Y + x *
                    (((((n[5] * x + n[4]) * x + n[3]) * x + n[2]) * x + n[1]) * x + n[0]) /
                    (((((d[5] * x + d[4]) * x + d[3]) * x + d[2]) * x + d[1]) * x + d[0]);
    return result;
}

template<class Real>
Real Alge_<Real>::hypot(Real a, Real b)
{
    a = abs(a);
    b = abs(b);
    if( a > b )
    {
        b /= a;
        return a*sqrt(1 + b*b);
    }
    if( b > 0 )
    {
        a /= b;
        return b*sqrt(1 + a*a);
    }
    return 0;
}

template<class Real>
tuple<bool,Real,Real> Alge_<Real>::solve(Real a, Real b, Real c, Real d, Real u, Real v)
{
    if(a*d == b*c) return make_tuple(false,Real(0),Real(0));

    Real x,y;
    if(abs(a) > abs(c))
    {
        Real ra = 1/a;
        d -= b*c*ra;
        v -= u*c*ra;
        y = v/d;
        x = (u-b*y)*ra;
    }
    else
    {
        Real rc = 1/c;
        b -= d*a*rc;
        u -= v*a*rc;
        y = u/b;
        x = (v-d*y)*rc;
    }
    return make_tuple(true,x,y);
}

template class Alge_<Float>;
template class Alge_<Double>;
template class Alge_<Quad>;

}