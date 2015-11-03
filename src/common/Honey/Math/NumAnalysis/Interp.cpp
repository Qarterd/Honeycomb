// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/Interp.h"
#include "Honey/Math/NumAnalysis/Polynomial.h"

namespace honey
{

template<class Real>
Real Interp_<Real>::linearAngle(Real t, Real angleStart, Real angleEnd, optional<int&> rotSign_)
{    
    int rotSign = 0;
    
    if (angleStart == angleEnd)
    {
        if (rotSign_) rotSign_ = rotSign;
        return angleStart;
    }
    
    Real dist = Trig::distanceAngle(angleStart, angleEnd);
    Real angleAmount = dist*t;
    
    Real angleRet = angleStart;
    
    //Take shortest direction around circle
    if (Alge::abs(angleStart - angleEnd) <= Real_::pi)
    {
        //We don't have to cross -pi -> pi boundary
        if (angleEnd < angleStart)
        {
            angleRet -= angleAmount;
            rotSign = -1;
            //Check if we passed desired angle and turn is done
            if (angleEnd > angleRet)
                angleRet = angleEnd;
        }
        else
        {
            angleRet += angleAmount;
            rotSign = 1;
            //Check if we passed desired angle and turn is done
            if (angleEnd < angleRet)
                angleRet = angleEnd;
        }
    }
    else
    {
        //We have to cross -pi -> pi boundary
        if (angleEnd < angleStart)
        {
            angleRet += angleAmount;
            rotSign = 1;
            if (angleRet > Real_::pi)
                angleRet -= Real_::piTwo;
            //Check if we passed desired angle and turn is done
            if (angleRet < 0 && angleEnd < angleRet)
                angleRet = angleEnd;
        }
        else
        {
            angleRet -= angleAmount;
            rotSign = -1;
            if (angleRet < -Real_::pi)
                angleRet += Real_::piTwo;
            //Check if we passed desired angle and turn is done
            if (angleRet > 0 && angleEnd > angleRet)
                angleRet = angleEnd;
        }
    }
    
    if (rotSign_) rotSign_ = rotSign;
    return angleRet;
}

template<class Real>
void Interp_<Real>::alignDir(Vec3& dir, const Vec3& targetDir, Real angleAmount, optional<int&> rotSign_)
{
    int rotSign = 0;
    if (dir == targetDir)
    {
        if (rotSign_) rotSign_ = rotSign;
        return;
    }
    Real targetAngle = Trig::atan2(targetDir.z, targetDir.x);
    Real dirAngle = Trig::atan2(dir.z, dir.x);
    Real dist = Trig::distanceAngle(dirAngle, targetAngle);
    dirAngle = dist > 0 ? linearAngle(angleAmount / dist, dirAngle, targetAngle, rotSign) : targetAngle;
    dir = dirAngle != targetAngle ? Vec3(Trig::cos(dirAngle), 0, Trig::sin(dirAngle)) : targetDir;
    if (rotSign_) rotSign_ = rotSign;
}

template<class Real>
auto Interp_<Real>::bezierRoots(Real y, Real v0, Real v1, Real v2, Real v3) -> tuple<Vec3, int>
{
    //Root finder is most stable if normalized to range [0,1]
    const Real eps = Real_::epsilon*10;
    Real norm_start = v0;
    Real norm_dist = Alge::abs(v3 - v0);
    if (!Alge::isNearZero(norm_dist, eps))
    {
        y = (y - norm_start)/norm_dist;
        v0 = 0;
        v1 = (v1 - norm_start)/norm_dist;
        v2 = (v2 - norm_start)/norm_dist;
        v3 = 1;
    }

    Vec3 roots;
    int count;
    tie(roots, count) = Polynomial<Real>::roots(Vec4(v0 - y,  3*(v1 - v0),  3*(v0 - 2*v1 + v2),  v3 - v0 + 3*(v1 - v2)), eps);
    //Must account for numeric error, ignore roots outside valid range [0,1]+eps
    const Real max = 1;
    int count_ = 0;
    for (int i = 0; i < count; ++i) if (Alge::isInRange(roots[i], eps, max+eps)) roots[count_++] = Alge::min(roots[i], max);
    return make_tuple(roots, count_);
}

template<class Real>
auto Interp_<Real>::bezierNormalizeHandles(const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& v3) -> tuple<Vec2, Vec2>
{
    //Handle deltas
    Vec2 h0 = v1 - v0;
    Vec2 h1 = v2 - v3;
    //Total time between keys
    Real len = v3.x - v0.x;
    //Time to handles
    Real len0 = Alge::abs(h0.x);
    Real len1 = Alge::abs(h1.x);
    Real lenTotal = len0 + len1;
    //Only need to normalize if handles overlap on time axis
    if (Alge::isNearZero(lenTotal) || lenTotal <= len) return make_tuple(v1, v2);
    //Remove time axis overlap in a way that preserves the ratio of handle deltas
    Real norm = len / lenTotal;
    return make_tuple(v0 + norm*h0, v3 + norm*h1);
}

template<class Real>
Real Interp_<Real>::bezierAtTime(Real time, const Vec2& v0, const Vec2& v1_, const Vec2& v2_, const Vec2& v3)
{
    assert(Alge::isInRange(time, v0.x, v3.x));
    Vec2 v1, v2;
    tie(v1, v2) = bezierNormalizeHandles(v0, v1_, v2_, v3);
    Vec3 roots = get<0>(bezierRoots(time, v0.x, v1.x, v2.x, v3.x));
    return bezier(roots.x, v0.y, v1.y, v2.y, v3.y);
}

template<class Real>
Real Interp_<Real>::bezierAngleAtTime(Real time, const Vec2& v0, const Vec2& v1_, const Vec2& v2_, const Vec2& v3_)
{
    Vec2 v1, v2, v3 = v3_;
    tie(v1, v2) = bezierNormalizeHandles(v0, v1_, v2_, v3);
    Real dist = Trig::alignAngle(v0.y, v3.y);
    Real c1 = v2.y - v3.y;      // save end-handle delta
    v3.y = v0.y + dist;         // move end-point to create shortest path (end-point is now non-normalized)
    v2.y = v3.y + c1;           // set end-handle at new end-point
    return Trig::normalizeAngle(bezierAtTime(time, v0, v1, v2, v3));
}

template<class Real>
void Interp_<Real>::bezierSubdiv(vector<Vec2>& cs, int index, Real t)
{
    // De Casteljau triangle matrix
    Vec2 mat[4][4];
    // Copy control points
    for (int j = 0; j < 4; ++j) mat[0][j] = cs[index+j];
    // Calc coefficients of triangle matrix
    for (int i = 1; i < 4; ++i)
    {
        for (int j = 0 ; j < 4 - i; ++j)
        {
            mat[i][j].x = mat[i-1][j].x*(1-t) + mat[i-1][j+1].x*t;
            mat[i][j].y = mat[i-1][j].y*(1-t) + mat[i-1][j+1].y*t;
        }
    }
    // Left segment, replace existing control points
    for (int j = 0; j < 4; ++j) cs[index+j] = mat[j][0];
    // Right segment, add 3 new control points
    for (int j = 1; j < 4; ++j) cs.insert(cs.begin() + index+3+j, mat[3-j][j]);
}

template<class Real>
Real Interp_<Real>::bezierSubdivAdapt(vector<Vec2>& cs, int index, Real tol)
{
    Real arc = 0;
    for (int i = index; i >= index; i -= 3)
    {
        //Get polygon length from 4 control points
        Real poly = 0;
        for (int j = 0; j < 3; ++j) poly += (cs[i+j+1] - cs[i+j]).length();
        //Get chord length from first control to last control
        Real chord = (cs[i+3] - cs[i]).length();
        //Done subdividing if near
        if (Alge::isNear(poly, chord, tol))
        {
            //Estimate arc length using average
            arc += (poly + chord) / 2;
            continue;
        }
        bezierSubdiv(cs, i, 0.5);
        //Recurse to right segment of subdivision
        i += 6;
    }
    return arc;
}

template class Interp_<Float>;
template class Interp_<Double>;
template class Interp_<Quad>;

}
