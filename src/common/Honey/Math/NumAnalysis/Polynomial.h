// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"
#include "Honey/Math/Alge/Trig.h"

namespace honey
{

/// Polynomial algorithms
/**
  * A polynomial is represented by a vector of coefficients `c`. The polynomial's degree is c.size()-1.
  * The lowest degree is at the first index. ex. \f$ c_2 x^2 + c_1 x + c_0 \f$
  */
template<class Real>
class Polynomial
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef Alge_<Real>                     Alge;
    typedef Trig_<Real>                     Trig;

public:
    typedef Vec<2, Real>                    Vec2;
    typedef Vec<3, Real>                    Vec3;
    typedef Vec<4, Real>                    Vec4;
    typedef Vec<5, Real>                    Vec5;
    typedef Vec<matrix::dynamic, Real>      Vec;

    /// Evaluate a polynomial at x
    template<class T>
    static Real eval(const VecBase<T>& c, Real x)
    {
        int degree = c.size()-1;
        Real res = degree >= 0 ? c[degree] : 0;
        for (auto i : range(degree-1, -1, -1))
        {
            res *= x;
            res += c[i];
        }
        return res;
    }

    /// Reduce the degree by eliminating all near-zero leading coefficients and by making the leading coefficient one.
    /**
      * \param c        Coefficients of the input polynomial
      * \param epsilon  Threshold to clamp to zero
      * \retval coeff   Coefficients of the compressed polynomial
      * \retval degree  Degree of the polynomial in range [-1, c.size()-1] (at degree 0 there is 1 coeff).
      */
    template<class T>
    static tuple<T, int> compress(const VecBase<T>& c, Real epsilon = Real_::epsilon)
    {
        T poly = c;
        int degree = poly.size()-1;

        for (auto i : range(degree, -1, -1))
            if (Alge::isNearZero(poly[i], epsilon)) --degree;
            else break;

        if (degree >= 0)
        {
            Real leadingInv = 1 / poly[degree];
            poly[degree] = 1;
            for (auto i : range(degree)) poly[i] *= leadingInv;
        }

        return make_tuple(poly, degree);
    }

    /// Get the derivative of a polynomial. Returns a polynomial with 1 degree less.
    template<class T>
    static auto derivative(const VecBase<T>& c) ->
        honey::Vec<(T::s_size == matrix::dynamic ? matrix::dynamic : T::s_size - 1), Real>
    {
        int degree = c.size()-1;
        honey::Vec<(T::s_size == matrix::dynamic ? matrix::dynamic : T::s_size - 1), Real> poly;
        poly.resize(degree);

        int i1 = 1;
        for (auto i0 : range(degree))
        {
            poly[i0] = i1*c[i1];
            ++i1;
        }
        return poly;
    }

    /// Find roots using an algebraic closed form expression. Solves the linear equation: \f$ c_1 x + c_0 = 0 \f$
    /**
      * \retval roots
      * \retval rootCount
      */
    static tuple<Real, int> roots(const Vec2& c, Real epsilon = Real_::epsilon);
    /// Solves the quadratic equation: \f$ c_2 x^2 + c_1 x + c_0 = 0 \f$
    static tuple<Vec2, int> roots(const Vec3& c, Real epsilon = Real_::epsilon);
    /// Solves the cubic equation: \f$ c_3 x^3 + c_2 x^2 + c_1 x + c_0 = 0 \f$
    static tuple<Vec3, int> roots(const Vec4& c, Real epsilon = Real_::epsilon);
    /// Solves the quartic equation: \f$ c_4 x^4 + c_3 x^3 + c_2 x^2 + c_1 x + c_0 = 0 \f$
    static tuple<Vec4, int> roots(const Vec5& c, Real epsilon = Real_::epsilon);

    /// Find roots of generic polynomial using bisection
    /**
      * This algorithm works by first recursively finding the roots of the polynomial's derivative.
      * The roots of the derivative are monotonic ranges over the polynomial, thus these ranges can be searched using bisection.
      */
    static tuple<Vec, int> roots(const Vec& c, Real epsilon = Real_::epsilon, int iterMax = 30);
    /// Find roots of generic polynomial within range using bisection
    static tuple<Vec, int> rootsInRange(const Vec& c, Real min, Real max, Real epsilon = Real_::epsilon, int iterMax = 30);

    /// Get lower and upper bounds of root magnitudes. Returns positive range or 0 if polynomial is constant (degree 0).
    template<class T>
    static tuple<Real,Real> rootBounds(const VecBase<T>& c, Real epsilon = Real_::epsilon)
    {
        T poly;
        int degree;
        tie(poly, degree) = compress(c, epsilon);

        if (degree <= 0) return make_tuple((Real)0,(Real)0); //Polynomial is constant

        Real upperMax = 0, lowerMax = 0;
        for (auto i : range(degree))
        {
            Real tmp = Alge::abs(poly[i]);
            if (tmp > upperMax) upperMax = tmp;
            tmp = Alge::abs(poly[i+1]);
            if (tmp > lowerMax) lowerMax = tmp;
        }

        //Leading is 1 because of compress
        Real constant = Alge::abs(poly[0]);
        return make_tuple(!Alge::isNearZero(constant) ? constant / (constant + lowerMax) : 0, 1 + upperMax);
    }
};

extern template class Polynomial<Float>;
extern template class Polynomial<Double>;
extern template class Polynomial<Quad>;

}
