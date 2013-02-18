// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Discrete.h"

namespace honey
{

/// Generate a random variate between min and max non-inclusive with uniform (flat) distribution
/**
  * This is a uniform distribution, so every value in range (min, max) has equal chance.
  * Construct without arguments to generate variates within range (0, 1) non-inclusive
  *
  * \param  min Minimum value. Range [-inf, inf]
  * \param  max Maximum value. Range [-inf, inf]
  * \retval x   Random variate. Range (min, max) non-inclusive
  */
template<class Real>
class Uniform_ : public RandomDist<Real>
{
    typedef RandomDist<Real> Super;
    RandomDist_imports();
    
public:
    Uniform_(RandomGen& gen)                        : Super(gen), _std(true)    { this->min = 0;    this->max = 1;                          }
    Uniform_(RandomGen& gen, Real min, Real max)    : Super(gen), _std(false)   { this->min = min;  this->max = max; assert(min <= max);    }

    /// Static function for standard distribution. Generate random real variate between 0 and 1 non-inclusive
    static Real nextStd(RandomGen& gen)             { error("Unsupported Real Type"); return 0; }

    virtual Real next() const                       { return _std ? nextStd(getGen()) : (max-min)*nextStd(getGen()) + min;  }
    virtual Real pdf(Real x) const                  { return Alge::isInRange(x,min,max) ? 1 / (max-min) : 0; }
    virtual Real cdf(Real x) const                  { if (x <= min) return 0; return x >= max ? Real(1) : (x - min) / (max - min); }
    virtual Real cdfInv(Real P) const               { if (P <= 0) return min-Real_::epsilon; return P >= 1 ? max-Real_::epsilon : min + P*(max-min); }
    virtual Real mean() const                       { return 0.5*(min+max); }
    virtual Real variance() const                   { return Alge::sqr(max-min) / 12; }
    
private:
    bool _std;
public:
    Real min;
    Real max;
};

/** \cond */
/// Specialization to return float between 0 and 1 non-inclusive
template<> inline Float Uniform_<Float>::nextStd(RandomGen& gen)
{
    union { float f; uint32 i; } res;
    #define FLOAT_SIG_MASK  0x007FFFFFU
    #define FLOAT_EXP_ZERO  0x3F800000U
    res.i = (Discrete_<uint32>::nextStd(gen) & FLOAT_SIG_MASK) | FLOAT_EXP_ZERO | 1;
    return res.f - 1.f;
}

/// Specialization to return double between 0 and 1 non-inclusive
template<> inline Double Uniform_<Double>::nextStd(RandomGen& gen)
{
    union { double f; uint64 i; } res;
    #define DOUBLE_SIG_MASK  0x000FFFFFFFFFFFFFULL
    #define DOUBLE_EXP_ZERO  0x3FF0000000000000ULL
    res.i = (Discrete_<uint64>::nextStd(gen) & DOUBLE_SIG_MASK) | DOUBLE_EXP_ZERO | 1;
    return res.f - 1.;
}

/// Specialization to return quad between 0 and 1 non-inclusive
template<> inline Quad Uniform_<Quad>::nextStd(RandomGen& gen)
{
    union { double f; uint64 i; } res;
    res.i = (Discrete_<uint64>::nextStd(gen) & DOUBLE_SIG_MASK) | DOUBLE_EXP_ZERO | 1;
    return res.f - 1.;
}
/** \endcond */

typedef Uniform_<Real>      Uniform;
typedef Uniform_<Float>     Uniform_f;
typedef Uniform_<Double>    Uniform_d;

extern template class Uniform_<Float>;
extern template class Uniform_<Double>;

}
