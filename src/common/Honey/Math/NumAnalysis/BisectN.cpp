// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/BisectN.h"
#include "Honey/Misc/ScopeGuard.h"

namespace honey
{

template<class Real, int Dim>
auto BisectN<Real,Dim>::root(const Funcs& funcs, const Vec& min, const Vec& max) -> tuple<bool, Vec>
{
    //Build root node and start recursion using node stack
    _funcs = &funcs;
    _root.fromZero();
    _minRes = Real_::inf;
    _nodes.push_back(Node());
    Node& node = _nodes.back();
    node.min = min;
    node.max = max;
    bool res = root();
    return make_tuple(res, _root);
}

template<class Real, int Dim>
bool BisectN<Real,Dim>::root()
{
    Node& node = _nodes.back();
    Vec& min = node.min;
    Vec& max = node.max;
    auto _ = ScopeGuard([&] { _nodes.pop_back(); });

    //Build corners
    for (auto i: range(childCount))
        _corners[i] = corner(min,max,i);

    //Eval funcs at corners
    for (auto fi: range(stdutil::size(*_funcs)))
        for (auto i: range(childCount))
            _funcsCorners[fi][i] = _funcs->at(fi)(_corners[i]);

    //For each corner, return true if all functions are zero
    for (auto i: range(childCount))
    {
        Real sum = reduce(_funcsCorners, Real(0), [&](Real a, auto& e) { return a + Alge::abs(e[i]); });
        if (sum >= _minRes) continue;
        //Best guess so far
        _minRes = sum;
        _root = _corners[i];
        if (Alge::isNearZero(_minRes, _tol)) return true;
    }

    //For each func, return false if same sign at all corners
    for (auto& funcCorners: _funcsCorners)
    {
        Real sign = reduce(funcCorners, Real(0), [&](Real a, Real e) { return a + Alge::sign(e); });
        if (Alge::abs(sign) == childCount) return false;
    }

    //Depth-first recursion through children
    if (stdutil::size(_nodes) == _depthMax) return false; //No recursion at max depth
    node.center = (min + max) / 2;
    for (auto i: range(childCount))
    {
        _nodes.push_back(Node());
        Node& child = _nodes.back();
        tie(child.min, child.max) = this->child(min, max, node.center, i);
        if (root()) return true;
    }

    return false;
}

template<class Real, int Dim>
auto BisectN<Real,Dim>::corner(const Vec& min, const Vec& max, int index) -> Vec
{
    Vec corner;
    for (auto i: range(dim))
    {
        corner[i] = (index & 1) ? max[i] : min[i];
        index >>= 1;
    }
    return corner;
}

template<class Real, int Dim>
auto BisectN<Real,Dim>::child(const Vec& min, const Vec& max, const Vec& center, int index) -> tuple<Vec,Vec>
{
    Vec cmin;
    Vec cmax;
    for (auto i: range(dim))
    {
        if (index & 1)
        {
            cmin[i] = center[i];
            cmax[i] = max[i];
        }
        else
        {
            cmin[i] = min[i];
            cmax[i] = center[i];
        }
        index >>= 1;
    }
    return make_tuple(cmin,cmax);
}

template class BisectN<Float, 2>;
template class BisectN<Float, 3>;
template class BisectN<Double, 2>;
template class BisectN<Double, 3>;

}