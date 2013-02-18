// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop
#ifdef HONEY_DX9

#include "Honey/Math/Alge/Vec/Vec4.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey
{

static D3DXVECTOR4*       DX(Vec4_f& v)          { return reinterpret_cast<D3DXVECTOR4*>(&v); }
static const D3DXVECTOR4* DX(const Vec4_f& v)    { return reinterpret_cast<const D3DXVECTOR4*>(&v); }

template<> Vec<4,Float> VecBase<Vec<4,Float>>::normalize(option<Real&> len) const
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
    D3DXVec4Normalize(DX(ret), DX(*this));
    return ret;
};

template class VecBase<Vec<4,Float>>;

}
/** \endcond */
#endif