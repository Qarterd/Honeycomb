// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/NumAnalysis/Bisect.h"
#include "Honey/Math/Alge/Vec/Vec4.h"

namespace honey
{

/// Find the root of a set of functions by the bisection method. ie. Finds (x,y,...) where all functions return 0.
/**
  * Bisection should only be used if the function to be evaluated is monotonic over the range [min,max],
  * ie. the function is either increasing or decreasing over the range.
  */
template<class Real, int Dim>
class BisectN
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef Alge_<Real>                     Alge;

public:
    typedef Vec<Dim, Real>          Vec;
    typedef function<Real (Vec)>    Func;
    typedef array<Func, Dim>        Funcs;

    static const int dim = Dim;

    /**
      * \param tol      Find root to within tolerance of zero
      * \param depthMax Tree is explored one path at a time using a stack, requires depthMax nodes.
      */
    BisectN(Real tol = Real_::zeroTol, int depthMax = 30)       : _tol(tol), _depthMax(depthMax) { _nodes.reserve(_depthMax); } 

    /// Find the root of a set of functions within bounds [min,max]
    /**
      * \param funcs
      * \param min      root lower bound
      * \param max      root upper bound
      * \retval found   if not found a best guess is returned
      * \retval root
      */
    tuple<bool,Vec> root(const Funcs& funcs, const Vec& min, const Vec& max);

private:
    static const int childCount = 1 << dim;

    struct Node
    {
        Vec min;
        Vec max;
        Vec center;
    };

    /// Recursive function that uses nodes array as stack
    bool root();

    /// Get corner vertex
    static Vec corner(const Vec& min, const Vec& max, int index);
    static tuple<Vec,Vec> child(const Vec& min, const Vec& max, const Vec& center, int index);

    Real _tol;
    int _depthMax;
    const Funcs* _funcs;
    Vec _root;
    Real _minRes;
    vector<Node> _nodes;
    array<Vec, childCount> _corners;
    array<array<Real, childCount>, dim> _funcsCorners;
};

extern template class BisectN<Float, 2>;
extern template class BisectN<Float, 3>;
extern template class BisectN<Double, 2>;
extern template class BisectN<Double, 3>;

}

