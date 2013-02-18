// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Trig.h"
#include "Honey/Math/Alge/Transform.h"

namespace honey
{

/// Interpolation math
template<class Real>
class Interp_ : mt::NoCopy
{
    typedef typename Numeral<Real>::Real_ Real_;
    typedef Alge_<Real>         Alge;
    typedef Trig_<Real>         Trig;
    typedef Vec<2,Real>         Vec2;
    typedef Vec<3,Real>         Vec3;
    typedef Vec<4,Real>         Vec4;
    typedef Quat_<Real>         Quat;
    typedef Matrix<4,4,Real>    Matrix4;
    typedef Transform_<Real>    Transform;

public:

    /// Linear interpolation.  t range is [0,1]
    template<class T>
    static T linear(Real t, const T& a, const T& b)                             { return a + t*(b-a); }
    static Quat linear(Real t, const Quat& a, const Quat& b)                    { return Quat::slerp(t, a, b); }

    static Transform linear(Real t, const Transform& a, const Transform& b)
    {
        if (t <= 0)
            return a;
        if (t >= 1)
            return b;

        return Transform(
            linear(t, a.getTrans(), b.getTrans()),
            (a.hasRot() || b.hasRot()) ? linear(t, a.getRot(), b.getRot()) : a.getRot(),
            (a.hasScale() || b.hasScale()) ? linear(t, a.getScale(), b.getScale()) : a.getScale(),
            (a.hasSkew() || b.hasSkew()) ? linear(t, a.getSkew(), b.getSkew()) : a.getSkew()
            );
    }

    /// Linearly interpolate angles along the shortest path.  Angles must be normalized. `t` range is [0,1]. rotSign is the direction rotated: (-ve, +ve, none) = (-1,1,0)
    static Real linearAngle(Real t, Real angleStart, Real angleEnd, optional<int&> rotSign = optnull);

    /// Align a normalized direction towards a target direction, rotating around the Y axis, stepping angleAmount.  rotSign is the direction rotated: (-ve, +ve, none) = (-1,1,0)
    static void alignDir(Vec3& dir, const Vec3& targetDir, Real angleAmount, optional<int&> rotSign = optnull);

    /// Triangular bary-centric interpolation.
    /**
      * Input -> Output:
      *
      *     (0,0)    -> x0          (1,0) -> x1         (0,1) -> x2
      *     1-f-g==0 -> line x1,x2  (f,0) -> line x0,x1 (0,g) -> line x0,x2
      */ 
    template<class T>
    static T baryCentric(Real f, Real g, const T& x0, const T& x1, const T& x2)
    {
        return (1-f-g)*x0 + f*x1 + g*x2;
    }

    static Quat baryCentric(Real f, Real g, const Quat& q0, const Quat& q1, const Quat& q2)
    {
        return Quat::baryCentric(f, g, q0, q1, q2);
    }

    static Transform baryCentric(Real f, Real g, const Transform& tm0, const Transform& tm1, const Transform& tm2)
    {
        Real t = f+g;
        return t != 0 ? linear(g/t, linear(t, tm0, tm1), linear(t, tm0, tm2)) : tm0;
    }

    /// Linearly blend a range of values by applying an associated weight to each value. If all weights are 0 then the first value is returned.
    template<class Range, class Seq>
    static auto blend(Range&& vals, Seq&& weights_) -> mt_elemOf(vals)
    {
        auto val = begin(vals), last = end(vals);
        auto weights = seqToIter(weights_);
        //Get first non-zero weight, if not found then return first value
        for (; val != last && *weights == 0; ++val, ++weights);
        if (val == last) return *begin(vals);
        //Blend the values
        mt_elemOf(vals) ret(*val++);
        Real weightAccum = *weights++;
        for (; val != last; ++val, ++weights)
        {
            weightAccum += *weights;
            Real weight = *weights / weightAccum;
            ret = linear(weight, ret, *val);
        }
        return ret;
    }

    /// Sin interpolation. 
    /**
      * \param t            distance along curve [0,1]
      * \param smoothIn     whether to accelerate into the curve starting at 0
      * \param smoothOut    whether to decelerate out of the curve ending at 1
      * \return             interpolated value in range [0,1]
      */
    static Real sin(Real t, bool smoothIn, bool smoothOut)
    {
        if (smoothIn && smoothOut)
            return 0.5 - (Trig::sin(Real_::piHalf + t*Real_::pi)/2);
        else if (smoothIn)
            return 1 - Trig::sin(Real_::piHalf + t*Real_::piHalf);
        else
            return Trig::sin(t*Real_::piHalf);
    }


    /// Gaussian / Normal distribution. The standard distribution parameters are (offset, scale) = (0, 1)
    static Real gaussian(Real x, Real offset, Real scale)
    {
        static const Real sqrtTwoPi = Alge::sqrt(Real_::piTwo);
        return Alge::exp( -Alge::sqr(x - offset) / (2 * Alge::sqr(scale)) ) / ( scale * sqrtTwoPi );
    }

    /// Perform gaussian for each element
    template<class T>
    static typename MatrixBase<T>::MatrixS gaussian(const MatrixBase<T>& x, const MatrixBase<T>& offset, const MatrixBase<T>& scale)
    {
        assert(x.size() == offset.size() && x.size() == scale.size());
        return map(x, offset, scale, typename MatrixBase<T>::MatrixS().resize(x.rows(), x.cols()),
            [](Real x, Real offset, Real scale) { return gaussian(x, offset, scale); });
    }

    /// Interpolate along a Bezier curve passing through v0 and v3, using handles (control points) v1 and v2.  The handles shape the curve and typically don't lie on it.
    /**
      * \param t    distance along curve [0-1]
      * \param v0   start point
      * \param v1   start handle
      * \param v2   end handle
      * \param v3   end point
      * \return     interpolated value
    */
    template<class T>
    static T bezier(Real t, const T& v0, const T& v1, const T& v2, const T& v3)
    {
        T c = 3 * (v1 - v0);
        T b = 3 * (v2 - v1) - c;
        T a = v3 - v0 - c - b; 
        Real tSqr = t * t;
        Real tCube = tSqr * t;
        return (a * tCube) + (b * tSqr) + (c * t) + v0;
    }

    /// Find roots of bezier function at y-intercept `y`
    /**
      * \retval roots
      * \retval rootCount
      */
    static tuple<Vec3, int> bezierRoots(Real y, Real v0, Real v1, Real v2, Real v3);
    
    /// Given a bezier curve with dim (time, value), normalize the handles (v1,v2) such that there is only 1 root at any point along the time axis.
    static tuple<Vec2, Vec2> bezierNormalizeHandles(const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& v3);
    
    /// Given a bezier curve with dim (time, value), get value on curve parameterized by `time` in range [0,1]
    static Real bezierAtTime(Real time, const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& v3);
    
    /// Similar to bezierAtTime() except the value is interpolated by taking the shortest angular path
    static Real bezierAngleAtTime(Real time, const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& v3);
    
    /// Subdivide a bezier curve segment at index (4 control points) by curve param 't' [0,1].  Replaces curve segment with equivalent left/right segments (7 control points)
    static void bezierSubdiv(vector<Vec2>& cs, int index, Real t);
    
    /// Adaptively subdivide a bezier curve segment at index (4 control points).  Subdivides curve segment until the arc length does not change more than the given tolerance.
    /**
     * \return Bezier curve arc length
     */
    static Real bezierSubdivAdapt(vector<Vec2>& cs, int index, Real tol = 0.01);

    /// Bezier 2D patch coefficient matrix generator
    /**
      * \param val      a 4x4 grid of control points to interpolate
      * \retval patch
      * \see            bezierPatch() to evaluate `patch`
      */
    static Matrix4 bezierPatchCoeff(const Matrix4& val)
    {
        // Bezier basis matrix
        static const Matrix4 m(             
          -1,   3, -3,  1,
           3,  -6,  3,  0,
          -3,   3,  0,  0,
           1,   0,  0,  0
           );
        static const Matrix4 t = m.transpose();
        return m * (val * t);
    }

    /// Bezier 2D patch interpolation
    /**
      * \param coeff    patch coefficient matrix
      * \param x        eval point X [0-1]
      * \param y        eval point Y [0-1]
      * \return         interpolated value
      * \see            bezierPatchCoeff() to generate `coeff`
      */
    static Real bezierPatch(const Matrix4& coeff, Real x, Real y)
    {
        return  x*( x*( x*(y * (y * (y * coeff( 0) + coeff( 1)) + coeff( 2)) + coeff( 3))
                        + (y * (y * (y * coeff( 4) + coeff( 5)) + coeff( 6)) + coeff( 7))
                      ) + (y * (y * (y * coeff( 8) + coeff( 9)) + coeff(10)) + coeff(11))
                  )     + (y * (y * (y * coeff(12) + coeff(13)) + coeff(14)) + coeff(15));
    }


    /// Interpolate along a Catmull-Rom curve passing through v1 and v2, using handles (control points) v0 and v3.  The spline (piecewise curve) will pass through all control points.
    /**
      * \param t    distance along curve [0-1]
      * \param v0   start handle
      * \param v1   start point
      * \param v2   end point
      * \param v3   end handle
      * \return     interpolated value
      */
    template<class T>
    static T catmull(Real t, const T& v0, const T& v1, const T& v2, const T& v3)
    {
        T c1 = -0.5*v0           +  0.5*v2           ;
        T c2 =      v0 + -2.5*v1 +    2*v2 + -0.5*v3 ;
        T c3 = -0.5*v0 +  1.5*v1 + -1.5*v2 +  0.5*v3 ;
        return (((c3*t + c2)*t +c1)*t + v1);
    }

    /// Catmull-rom 2D patch coefficient matrix generator
    /**
      * \param val      a 4x4 grid of control points to interpolate
      * \retval patch
      * \see            catmullPatch() to evaluate `patch`
      */
    static Matrix4 catmullPatchCoeff(const Matrix4& val)
    {
        // Catmull-Rom basis matrix
        static const Matrix4 m(             
          -0.5,  1.5, -1.5,  0.5,
           1,   -2.5,    2, -0.5,
          -0.5,    0,  0.5,    0,
           0,      1,    0,    0
           );
        static const Matrix4 t = m.transpose();

        return m * (val * t);
    }

    /// Catmull-rom 2D patch interpolation
    /**
      * \param coeff    patch coefficient matrix
      * \param x        eval point X [0-1]
      * \param y        eval point Y [0-1]
      * \return         interpolated value
      * \see            catmullPatchCoeff() to generate `coeff`
      */
    static Real catmullPatch(const Matrix4& coeff, Real x, Real y)
    {
        return  x*( x*( x*(y * (y * (y * coeff( 0) + coeff( 1)) + coeff( 2)) + coeff( 3))
                        + (y * (y * (y * coeff( 4) + coeff( 5)) + coeff( 6)) + coeff( 7))
                      ) + (y * (y * (y * coeff( 8) + coeff( 9)) + coeff(10)) + coeff(11))
                  )     + (y * (y * (y * coeff(12) + coeff(13)) + coeff(14)) + coeff(15));
    }

};

typedef Interp_<Real>   Interp;
typedef Interp_<Float>  Interp_f;
typedef Interp_<Double> Interp_d;

extern template class Interp_<Float>;
extern template class Interp_<Double>;
extern template class Interp_<Quad>;

}

