// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop
#ifdef HONEY_DX9

#include "Honey/Math/Alge/Vec/Vec3.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey
{

static D3DXVECTOR3*       DX(Vec3_f& v)          { return reinterpret_cast<D3DXVECTOR3*>(&v); }
static const D3DXVECTOR3* DX(const Vec3_f& v)    { return reinterpret_cast<const D3DXVECTOR3*>(&v); }

template<> Vec<3,Float> VecBase<Vec<3,Float>>::normalize(option<Real&> len) const
{
    if (len)
    {
        Real l = length();
        if (l > RealT::zeroTol)
        {
            len = l;
            return *this / l;
        }
        len = 0;
        return VecS().fromZero();
    }

    VecS ret;
    D3DXVec3Normalize(DX(ret), DX(*this));
    return ret;
};

template class VecBase<Vec<3,Float>>;

}
/** \endcond */
#endif