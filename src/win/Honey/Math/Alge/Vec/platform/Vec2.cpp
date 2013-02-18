// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop
#ifdef HONEY_DX9

#include "Honey/Math/Alge/Vec/Vec2.h"
#include "Honey/Math/Alge/Alge.h"

/** \cond */
namespace honey
{

static D3DXVECTOR2*       DX(Vec2_f& v)          { return reinterpret_cast<D3DXVECTOR2*>(&v); }
static const D3DXVECTOR2* DX(const Vec2_f& v)    { return reinterpret_cast<const D3DXVECTOR2*>(&v); }

template<> Vec<2,Float> VecBase<Vec<2,Float>>::normalize(option<Real&> len) const
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
    D3DXVec2Normalize(DX(ret), DX(*this));
    return ret;
};

template class VecBase<Vec<2,Float>>;

}
/** \endcond */
#endif
