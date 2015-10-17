// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Alge/Transform.h"

namespace honey
{

template<class Real>
Transform_<Real> Transform_<Real>::inverse() const
{
    Transform_ tm = Transform_(mt::tag<0>());

    tm._bTrans = _bTrans;
    tm._bRot = _bRot;
    tm._bScale = _bScale;
    tm._bUniformScale = _bUniformScale;
    tm._bSkew = _bSkew || (!_bUniformScale && _bRot);

    tm._trans = -_trans;
    tm._rot = _rot.inverse();
    tm._scale = _bScale ? _scale.elemInverse() : _scale;
    tm._skew = tm._bSkew ? _rot * _skew : _skew;

    if (_bTrans)
    {
        if (_bScale)
            tm._trans = tm._bSkew ? tm._skew * (tm._skew.inverse()*tm._trans).elemMul(tm._scale) : tm._trans.elemMul(tm._scale);
        if (_bRot)
            tm._trans = tm._rot * tm._trans;
    }

    return tm;
}

template<class Real>
Transform_<Real> Transform_<Real>::operator*(const Transform_& tm) const
{
    Transform_ ret = Transform_(mt::tag<0>());

    if (tm._bTrans)
    {
        ret._bTrans = true;
        ret._trans = operator*(tm._trans);
    }
    else
    {
        ret._bTrans = _bTrans;
        ret._trans = _trans;
    }

    if (tm._bRot)
    {
        ret._bRot = true;
        if (_bRot)
            ret._rot = _rot * tm._rot;
        else
            ret._rot = tm._rot;
    }
    else
    {
        ret._bRot = _bRot;
        ret._rot = _rot;
    }

    if (tm._bScale)
    {
        ret._bScale = true;
        if (_bScale)
        {
            ret._scale = _scale.elemMul(tm._scale);
            if (_bUniformScale)
            {
                ret._bUniformScale = tm._bUniformScale;
                ret._bSkew = tm._bSkew;
                ret._skew = tm._skew;
            }
            else if (tm._bUniformScale)
            {
                ret._bUniformScale = false;
                ret._bSkew = _bSkew || tm._bRot;
                ret._skew = ret._bSkew ? tm._rot.inverse() * _skew : _skew;
            }
            else  //!_bUniformScale && !tm._bUniformScale
            {
                assert(!_bSkew && !tm._bRot && !tm._bSkew, "Can't concatenate skews");
                ret._bUniformScale = false;
                ret._bSkew = _bSkew;
                ret._skew = _skew;
            }
        }
        else
        {
            ret._bUniformScale = tm._bUniformScale;
            ret._bSkew = tm._bSkew;
            ret._scale = tm._scale;
            ret._skew = tm._skew;
        }
    }
    else
    {
        ret._bScale = _bScale;
        ret._bUniformScale = _bUniformScale;
        ret._bSkew = _bSkew || (!_bUniformScale && tm._bRot);
        ret._scale = _scale;
        ret._skew = ret._bSkew ? tm._rot.inverse() * _skew : _skew;
    }

    return ret;
}

template class Transform_<Float>;
template class Transform_<Double>;

}