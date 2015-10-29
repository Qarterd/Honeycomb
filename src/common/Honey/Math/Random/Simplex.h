// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Vec4.h"
#include "Honey/Math/Random/Gen.h"

namespace honey
{

/// Methods common to all simplex noise dimensions
template<class Subclass, int Dim, class Real>
class SimplexNoiseCommon
{
public:
    typedef Vec<Dim,Real> Vec;
    
    SimplexNoiseCommon(RandomGen& gen, int size);
    
protected:
    typedef Alge_<Real> Alge;

    int _size;
    vector<int> _perm;
};

/// Generate smooth noise over space. Implementation of Perlin's "Simplex Noise" generator.
/**
  * \class SimplexNoise
  * 
  * \tparam Dim     Dimension of field, range [1,4]
  */
template<int Dim, class Real>
class SimplexNoise : SimplexNoiseCommon<SimplexNoise<Dim,Real>, Dim, Real>
{
    typedef SimplexNoiseCommon<SimplexNoise<Dim,Real>, Dim, Real> Super;
    using typename Super::Alge;
public:
    using typename Super::Vec;

    /**
      * \param gen      Generator used to create noise. The generator is only used during construction.
      * \param size     Size of noise field. A power of two is recommended for speed.
      *                 Field range is [0, size], mirrored about the origin and wrapping for values larger than `size`.
      */
    SimplexNoise(RandomGen& gen, int size = 256);
    
    /// Get value at point in noise field. Returns noise in range [-1, 1].
    /** 
      * \param x    Point in noise field
      * \param dx   If specified, gradient (spacial derivative) at point `x` will be returned here.
      */
    Real noise(const Vec& x, optional<Vec&> dx = optnull);
};

/** \cond */
template<class Real>
class SimplexNoise<1, Real> : SimplexNoiseCommon<SimplexNoise<1,Real>, 1, Real>
{
    typedef SimplexNoiseCommon<SimplexNoise<1,Real>, 1, Real> Super;
    using typename Super::Alge;
public:
    using typename Super::Vec;
    
    SimplexNoise(RandomGen& gen, int size = 256)            : Super(gen, size) {}
    Real noise(const Vec& x, optional<Vec&> dx = optnull);
private:
    void grad(int hash, Real& gx);
};

template<class Real>
class SimplexNoise<2, Real> : SimplexNoiseCommon<SimplexNoise<2,Real>, 2, Real>
{
    typedef SimplexNoiseCommon<SimplexNoise<2,Real>, 2, Real> Super;
    using typename Super::Alge;
public:
    using typename Super::Vec;
    
    SimplexNoise(RandomGen& gen, int size = 256)            : Super(gen, size) {}
    Real noise(const Vec& x, optional<Vec&> dx = optnull);
private:
    void grad(int hash, Real& gx, Real& gy);
};

template<class Real>
class SimplexNoise<3, Real> : SimplexNoiseCommon<SimplexNoise<3,Real>, 3, Real>
{
    typedef SimplexNoiseCommon<SimplexNoise<3,Real>, 3, Real> Super;
    using typename Super::Alge;
public:
    using typename Super::Vec;
    
    SimplexNoise(RandomGen& gen, int size = 256)            : Super(gen, size) {}
    Real noise(const Vec& x, optional<Vec&> dx = optnull);
private:
    void grad(int hash, Real& gx, Real& gy, Real& gz);
};

template<class Real>
class SimplexNoise<4, Real> : SimplexNoiseCommon<SimplexNoise<4,Real>, 4, Real>
{
    typedef SimplexNoiseCommon<SimplexNoise<4,Real>, 4, Real> Super;
    using typename Super::Alge;
public:
    using typename Super::Vec;
    
    SimplexNoise(RandomGen& gen, int size = 256)            : Super(gen, size) {}
    Real noise(const Vec& x, optional<Vec&> dx = optnull);
private:
    void grad(int hash, Real& gx, Real& gy, Real& gz, Real& gw);
};
/** \endcond */

extern template class SimplexNoise<1, Float>;
extern template class SimplexNoise<2, Float>;
extern template class SimplexNoise<3, Float>;
extern template class SimplexNoise<4, Float>;

extern template class SimplexNoise<1, Double>;
extern template class SimplexNoise<2, Double>;
extern template class SimplexNoise<3, Double>;
extern template class SimplexNoise<4, Double>;

}
