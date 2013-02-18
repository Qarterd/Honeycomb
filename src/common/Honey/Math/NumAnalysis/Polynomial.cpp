// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/NumAnalysis/Polynomial.h"
#include "Honey/Math/NumAnalysis/Bisect.h"

namespace honey
{

template<class Real>
auto Polynomial<Real>::roots(const Vec2& c, Real epsilon) -> tuple<Real, int>
{
    return !Alge::isNearZero(c[1], epsilon) ? make_tuple(-c[0]/c[1], 1) : make_tuple((Real)0, 0);
}

template<class Real>
auto Polynomial<Real>::roots(const Vec3& c, Real epsilon) -> tuple<Vec2, int>
{
    auto root = Vec2().fromZero();

    if (Alge::isNearZero(c[2], epsilon))
    {
        // Polynomial is linear    
        int count;
        tie(root[0], count) = roots(Vec2(c), epsilon);
        return make_tuple(root, count);
    }

    Real discr = c[1]*c[1] - (4)*c[0]*c[2];
    if (Alge::isNearZero(discr, epsilon))
        discr = 0;

    if (discr < 0)
        return make_tuple(root, 0);

    Real tmp = 0.5 / c[2];

    if (discr > 0)
    {
        discr = Alge::sqrt(discr);
        root[0] = tmp*(-c[1] - discr);
        root[1] = tmp*(-c[1] + discr);
        return make_tuple(root, 2);
    }
    
    root[0] = -tmp*c[1];
    return make_tuple(root, 1);
}

template<class Real>
auto Polynomial<Real>::roots(const Vec4& c_, Real epsilon) -> tuple<Vec3, int>
{
    Vec4 c = c_;
    auto root = Vec3().fromZero();

    if (Alge::isNearZero(c[3], epsilon))
    {
        // Polynomial is quadratic
        auto seg = root.template segment<2>(0);
        int count;
        tie(seg, count) = roots(Vec3(c), epsilon);
        return make_tuple(root, count);
    }

    // Make polynomial monic, x^3+c[2]*x^2+c[1]*x+c[0]
    Real invC3 = 1 / c[3];
    c[0] *= invC3;
    c[1] *= invC3;
    c[2] *= invC3;

    // Convert to y^3+a*y+b = 0 by x = y-c[2]/3
    const Real third = 1 / (Real)3;
    const Real twentySeventh = 1 / (Real)27;
    Real offset = third*c[2];
    Real a = c[1] - c[2]*offset;
    Real b = c[0]+c[2]*(2*c[2]*c[2] - 9*c[1])*twentySeventh;
    Real halfB = 0.5*b;

    Real discr = halfB*halfB + a*a*a*twentySeventh;
    if (Alge::isNearZero(discr, epsilon))
        discr = 0;

    if (discr > 0)  // 1 real, 2 complex roots
    {
        discr = Alge::sqrt(discr);
        Real temp = -halfB + discr;
        if (temp >= 0)
            root[0] = Alge::pow(temp, third);
        else
            root[0] = -Alge::pow(-temp, third);
        temp = -halfB - discr;
        if (temp >= 0)
            root[0] += Alge::pow(temp, third);
        else
            root[0] -= Alge::pow(-temp, third);
        root[0] -= offset;
        return make_tuple(root, 1);
    }
    else if (discr < 0) 
    {
        const Real sqrt3 = Alge::sqrt(3);
        Real dist = Alge::sqrt(-third*a);
        Real angle = third * Trig::atan2(Alge::sqrt(-discr), -halfB);
        Real cs = Trig::cos(angle);
        Real sn = Trig::sin(angle);
        root[0] = 2*dist*cs - offset;
        root[1] = -dist*(cs + sqrt3*sn) - offset;
        root[2] = -dist*(cs - sqrt3*sn) - offset;
        return make_tuple(root, 3);
    }

    // discr == 0
    Real temp;
    if (halfB >= 0)
        temp = -Alge::pow(halfB, third);
    else
        temp = Alge::pow(-halfB, third);
    root[0] = 2*temp - offset;
    root[1] = -temp - offset;
    root[2] = root[1];
    return make_tuple(root, 3);
}

template<class Real>
auto Polynomial<Real>::roots(const Vec5& c_, Real epsilon) -> tuple<Vec4, int>
{
    Vec5 c = c_;
    auto root = Vec4().fromZero();

    if (Alge::isNearZero(c[4], epsilon))
    {
        // Polynomial is cubic
        auto seg = root.template segment<3>(0);
        int count;
        tie(seg, count) = roots(Vec4(c.template segment<4>(0)), epsilon);
        return make_tuple(root, count);
    }

    // Make polynomial monic, x^4+c[3]*x^3+c[2]*x^2+c[1]*x+c[0]
    Real invC4 = (1)/c[4];
    c[0] *= invC4;
    c[1] *= invC4;
    c[2] *= invC4;
    c[3] *= invC4;

    // Reduction to resolvent cubic polynomial y^3+r2*y^2+r1*y+r0 = 0
    Real r0 = -c[3]*c[3]*c[0] + 4*c[2]*c[0] - c[1]*c[1];
    Real r1 = c[3]*c[1] - 4*c[0];
    Real r2 = -c[2];
    Real y = get<0>(roots(Vec4(r0, r1, r2, 1), epsilon))[0];  // always produces at least one root

    int count = 0;
    Real discr = 0.25*c[3]*c[3] - c[2] + y;
    if (Alge::isNearZero(discr, epsilon))
        discr = 0;

    if (discr > 0) 
    {
        Real r = Alge::sqrt(discr);
        Real t1 = 0.75*c[3]*c[3] - r*r - 2*c[2];
        Real t2 = (4*c[3]*c[2] - 8*c[1] - c[3]*c[3]*c[3]) / (4*r);

        Real tPlus = t1 + t2;
        Real tMinus = t1 - t2;
        if (Alge::isNearZero(tPlus, epsilon)) 
            tPlus = 0;
        if (Alge::isNearZero(tMinus, epsilon)) 
            tMinus = 0;

        if (tPlus >= 0)
        {
            Real d = Alge::sqrt(tPlus);
            root[count++] = -0.25*c[3] + 0.5*(r + d);
            root[count++] = -0.25*c[3] + 0.5*(r - d);
        }
        if (tMinus >= 0)
        {
            Real e = Alge::sqrt(tMinus);
            root[count++] = -0.25*c[3] + 0.5*(e - r);
            root[count++] = -0.25*c[3] - 0.5*(e + r);
        }
    }
    else if (discr == 0)
    {
        Real t2 = y*y - 4*c[0];
        if (t2 >= -epsilon) 
        {
            // round to zero
            if (t2 < 0) t2 = 0;
            t2 = 2*Alge::sqrt(t2);
            Real t1 = 0.75*c[3]*c[3] - 2*c[2];
            Real tPlus = t1 + t2;

            if (tPlus >= epsilon)
            {
                Real d = Alge::sqrt(tPlus);
                root[count++] = -0.25*c[3] + 0.5*d;
                root[count++] = -0.25*c[3] - 0.5*d;
            }
            Real tMinus = t1 - t2;
            if (tMinus >= epsilon) 
            {
                Real e = Alge::sqrt(tMinus);
                root[count++] = -0.25*c[3] + 0.5*e;
                root[count++] = -0.25*c[3] - 0.5*e;
            }
        }
    }

    return make_tuple(root, count);
}

template<class Real>
auto Polynomial<Real>::roots(const Vec& c, Real epsilon, int iterMax) -> tuple<Vec, int>
{
    auto bounds = rootBounds(c, epsilon);
    return rootsInRange(c, -get<1>(bounds), get<1>(bounds), epsilon, iterMax);
}

template<class Real>
auto Polynomial<Real>::rootsInRange(const Vec& c, Real min, Real max, Real epsilon, int iterMax) -> tuple<Vec, int>
{
    int degree = c.size()-1;
    auto root = Vec(degree).fromZero();
    int count = 0;

    Bisect<Real> bisect(epsilon, iterMax);
    function<Real (Real)> eval_c = bind(eval<Vec>, c, _1);

    if (degree <= 0) return make_tuple(root, count); //Polynomial is constant

    if (degree == 1)
    {
        auto res = bisect.root(eval_c, min, max);
        if (get<0>(res)) root[count++] = get<1>(res);
        return make_tuple(root, count);
    }

    // Get roots of derivative polynomial
    Vec d = derivative(c);
    Vec dRoot;
    int dCount;
    tie(dRoot, dCount) = rootsInRange(d, min, max, epsilon, iterMax);

    if (dCount > 0)
    {
        // Must avoid duplicates: if the root lies on a boundary then it is possible to find the same root twice
        // Find root on [min, root[0]]
        auto res = bisect.root(eval_c, min, dRoot[0]);
        if (get<0>(res)) root[count++] = get<1>(res);

        // Find root on [root[i], root[i+1]] for 0 <= i <= count-2
        for (auto i : range(dCount-1))
        {
            auto res = bisect.root(eval_c, dRoot[i], dRoot[i+1]);
            if (get<0>(res) && (!count || !Alge::isNear(root[count-1], get<1>(res), epsilon)))
                root[count++] = get<1>(res);
        }

        // Find root on [root[count-1], max]
        res = bisect.root(eval_c, dRoot[dCount-1], max);
        if (get<0>(res) && (!count || !Alge::isNear(root[count-1], get<1>(res), epsilon)))
            root[count++] = get<1>(res);
    }
    else
    {
        // Polynomial is monotonic on [min, max], has at most one root
        auto res = bisect.root(eval_c, min, max);
        if (get<0>(res)) root[count++] = get<1>(res);
    }

    return make_tuple(root, count);
}

template class Polynomial<Float>;
template class Polynomial<Double>;
template class Polynomial<Quad>;

}
