// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/LinearLeastSqr.h"

namespace honey
{

template<class Real>
void LinearLeastSqr<Real>::calc(const Matrix& x, const Vec& y, Vec& b)
{
    assert(x.rows() == y.rows());
    b.resize(x.cols());
    _svd.calc(x).solve(y, b);
}

template<class Real>
void LinearLeastSqr<Real>::calc(const Matrix& x, const Vec& y, const Vec& w, Vec& b)
{
    assert(x.rows() == y.rows() && x.rows() == w.rows());
    //The equation we have to solve here is: (Xt*w*X)*b = Xt*w*y
    //But our weights are a diagonal of inverse variances, not a full inverse covariance matrix
    //So we can simply pre-apply sqrt(weights) to X and y: (Xt*X)*b = Xt*y
    //Now we have the standard "normal equation" that can be solved using SVD
    _w.resize(w.size());
    map(w, _w, [](Real w) { return Alge::sqrt(w); });
    //Apply weights to rows of X
    _x.resize(x.rows(),x.cols());
    for (auto i : range(x.rows()))
        map(range(x.iter(i,0), x.iter(i+1,0)), _x.iter(i,0), [&](Real x) { return x*_w[i]; });
    //Apply weights to y
    _y.resize(y.rows());
    map(y, _w, _y, [](Real y, Real w) { return y*w; });
    //Solve: X*b = y
    calc(_x, _y, b);
}

template<class Real>
void LinearLeastSqr<Real>::calc(const Matrix& x, const Vec& y, const Vec& w, const Matrix& c, const Vec& d, Vec& b)
{
    assert(x.rows() == y.rows() && x.rows() == w.rows());
    assert(c.cols() == x.cols());
    assert(c.rows() == d.rows());

    sdt k = c.rows(), n = c.cols();
    assert(k < n, "Too many constraints");
    /*
      * C^T = Q*|R|
      *        |0|
      * XQ = [X0, X1]
      * R^T*y0 = d
      * X1*y1 = y - X0*y0
      * b = Q*|y0|
      *      |y1|
      */
    c.transpose(_ct);
    _qrd.calc(_ct, Qrd::Mode::full);
    _qrd.r().transpose(_rt);
    x.mul(_qrd.q(), _xq);
    auto x0 = _xq.block(0,0,_xq.rows(),k);
    auto x1 = _xq.block(0,k,_xq.rows(),n-k);
    BackSub::solveFwd(_rt, d, _y0);
    x0.mul(_y0, _yTmp);
    y.sub(_yTmp, _yTmp);
    calc(x1, _yTmp, w, _y1);
    _yTmp.resize(n);
    _yTmp.segment(0,k) = _y0;
    _yTmp.segment(k,n-k) = _y1;
    _qrd.q().mul(_yTmp, b);
}

template class LinearLeastSqr<Float>;
template class LinearLeastSqr<Double>;

}