// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Alge/Trig.h"
#include "Honey/Math/NumAnalysis/Interp.h"

namespace honey
{

template<class Real>
void SinTable<Real>::resize(int size)
{
    if (size == _size) return;

    _size = size;
    _sin.resize(_size);
    _asin.resize(_size);
    _radToSin = (_size-1) / Real_::piHalf;
    _xToAsin = _size-1;

    for (int i = 0; i < _size; ++i)
    {
        _sin[i] = Real_::sin(i*Real_::piHalf / (_size-1));
        _asin[i] = Real_::asin(Real(i) / (_size-1));
    }
}

template<class Real>
Real SinTable<Real>::sin(Real x) const
{
    //Take arbitrary angle and bound between [0, pi*2)
    Real xn = Alge::mod(x, Real_::piTwo);
    if (xn < 0) xn = Real_::piTwo + xn;
    //Sin table uses symmetry and defines one quarter [0,pi/2].  Apply transform for other quarters.
    if (xn < Real_::pi)
        return xn < Real_::piHalf ? linear(_sin, xn*_radToSin) : linear(_sin, (Real_::pi-xn)*_radToSin);
    else
        return xn < Real_::piAndHalf ? -linear(_sin, (xn-Real_::pi)*_radToSin) : -linear(_sin, (Real_::piTwo-xn)*_radToSin);
}

template<class Real>
Real SinTable<Real>::asin(Real x) const
{
    //Asin table uses symmetry and defines values for [0,1].  Input is [-1,1]
    if (x > 1 || x < -1) return Real_::nan;
    return x >= 0 ? linear(_asin, x*_xToAsin) : -linear(_asin, -x*_xToAsin);
}

template<class Real>
Real SinTable<Real>::atan2(Real y, Real x) const
{
    Real xAbs = Alge::abs(x);
    Real yAbs = Alge::abs(y);
        
    if (yAbs <= Real_::zeroTol)
        return (x >= 0) ? 0 : Real_::pi;
        
    if (xAbs <= Real_::zeroTol)
        return (y > 0) ? Real_::piHalf : -Real_::piHalf;
        
    //Switch the axes and use symmetry for atan if divisor is too small
    if (xAbs < yAbs)
    {
        Real z = atan(xAbs / yAbs);
        if (y > 0)
            return Real_::piHalf + ((x < 0) ? z : -z);
        else
            return -Real_::piHalf + ((x > 0) ? z : -z);
    }
        
    Real z = atan(yAbs / xAbs);
    if (x > 0)
        return (y > 0) ? z : -z;
    else
        return (y > 0) ? Real_::pi - z : z - Real_::pi;
}

template class SinTable<Float>;
template class SinTable<Double>;
template class SinTable<Quad>;


template<class Real>
void Trig_<Real>::enableSinTable(bool enable, int size)
{
    auto& t = inst();
    t._tableEnable = enable;
    if (!enable) return;
    if (size < 0) size = t._table.size() ? t._table.size() : tableSizeDefault;
    t._table.resize(size);
}

template class Trig_<Float>;
template class Trig_<Double>;
template class Trig_<Quad>;

}
