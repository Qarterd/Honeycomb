// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once
#ifdef HONEY_DX9

namespace honey
{

template<> Vec<4,Float> VecBase<Vec<4,Float>>::normalize(option<Real&> len) const;

}

#endif