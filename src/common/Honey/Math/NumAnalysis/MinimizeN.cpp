// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/MinimizeN.h"

namespace honey
{

template<class Real, int Dim>
auto MinimizeN<Real,Dim>::calc(const Func& func, const Vec& min, const Vec& max, const Vec& init_) -> tuple<Vec,Real>
{
    Vec init = init_;

    // The initial guess
    Real fMin = func(init);
    Vec tMin = init;
    
    //Function for 1D minimize
    Vec* cur;
    auto minFunc = [&](Real t) { return func(tMin + t * *cur); };

    // Initialize the direction set to the standard Euclidean basis
    std::array<Vec, dim> dirsStorage;
    std::array<Vec*, dim> dirs;
    for (auto i : range(dim))
    {
        dirs[i] = &dirsStorage[i];
        dirs[i]->fromAxis(i);
    }

    Real ell0, ell1, ellMin;
    for (auto _ : range(_iterMax))
    {
        mt_unused(_);
        // Find minimum in each direction and update current location
        for (auto i : range(dim))
        {
            cur = dirs[i];
            tie(ell0, ell1) = calcDomain(tMin, *cur, min, max);
            tie(ellMin, fMin) = _minimize.calc(minFunc, ell0, ell1, 0);
            tMin += ellMin * *cur;
        }

        // Get the conjugate vector and its length
        // Record the direction of max length
        Vec conj;
        Real length = 0;
        Real maxLen = -Real_::max;
        int maxDir = 0;

        for (auto i : range(dim))
        {
            conj[i] = tMin[i] - init[i];
            Real len = conj[i]*conj[i];
            length += len;
            if (len > maxLen) { maxLen = len; maxDir = i; }
        }

        length = Alge::sqrt(length);
        if (length <= _tol) break; // New position did not change significantly from old one

        // Normalize conjugate
        Real invlength = 1 / length;
        conj *= invlength;

        // Minimize in conjugate direction
        cur = &conj;
        tie(ell0, ell1) = calcDomain(tMin, *cur, min, max);
        tie(ellMin, fMin) = _minimize.calc(minFunc, ell0, ell1, fMin);
        tMin += ellMin * *cur;

        // Replace most successful direction with conjugate direction
        *dirs[maxDir] = conj;
        // Cycle directions
        cur = dirs[0];
        for (auto i : range(dim-1)) dirs[i] = dirs[i+1];
        dirs[dim-1] = cur;
         
        // Set start point for next iteration
        init = tMin;
    }

    return make_tuple(tMin, fMin);
}

template<class Real, int Dim>
tuple<Real, Real> MinimizeN<Real,Dim>::calcDomain(const Vec& v, const Vec& dir, const Vec& min, const Vec& max)
{
    Real ell0 = -Real_::max;
    Real ell1 = Real_::max;

    for (auto i : range(dim))
    {
        Real b0 = min[i] - v[i];
        Real b1 = max[i] - v[i];
        if (dir[i] > 0)
        {
            // The valid t-interval is [b0,b1]
            Real inv = 1 / dir[i];
            b0 *= inv;
            if (b0 > ell0) ell0 = b0;
            b1 *= inv;
            if (b1 < ell1) ell1 = b1;
        }
        else if (dir[i] < 0)
        {
            // The valid t-interval is [b1,b0]
            Real inv = 1 / dir[i];
            b0 *= inv;
            if (b0 < ell1) ell1 = b0;
            b1 *= inv;
            if (b1 > ell0) ell0 = b1;
        }
    }

    // Correction if numerical errors lead to values nearly zero
    if (ell0 > 0) ell0 = 0;
    if (ell1 < 0) ell1 = 0;

    return make_tuple(ell0, ell1);
}

template class MinimizeN<Float, 2>;
template class MinimizeN<Float, 3>;
template class MinimizeN<Double, 2>;
template class MinimizeN<Double, 3>;

}