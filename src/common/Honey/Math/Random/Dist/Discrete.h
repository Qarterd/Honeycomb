// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Dist/Dist.h"

namespace honey
{

/// Generate random integer variate between min and max inclusive with uniform (flat) distribution
/**
  * This is a uniform distribution, so every value in range [min, max] has equal chance.
  *
  * Construct without arguments to generate variates within entire integer range (up to 64 bits), variate may be negative.
  *
  * \param  min Minimum value. Range [-IntMax, IntMax]
  * \param  max Maximum value. Range [-IntMax, IntMax]
  * \retval x   Random integer variate. Range (min, max) non-inclusive
  */
template<class Int>
class Discrete_ : public RandomDist<typename Numeral<Int>::Real>
{
    typedef RandomDist<typename Numeral<Int>::Real> Super;
    RandomDist_imports();
    typedef typename Numeral<Int>::Real Real;
    
public:
    Discrete_(RandomGen& gen)                       : Super(gen), _std(true)    { this->min = numeral<Int>().min(); this->max = numeral<Int>().max(); assert(min <= max); }
    Discrete_(RandomGen& gen, Int min, Int max)     : Super(gen), _std(false)   { this->min = min;                  this->max = max;                  assert(min <= max); }

    /// Static function for standard distribution. Generate a random variate within full integer range (up to 64 bits) with uniform (flat) distribution
    static Int nextStd(RandomGen& gen)              { return static_cast<Int>(gen.next()); }

    virtual Real next() const                       { return static_cast<Real>(nextInt()); }
    virtual Real pdf(Real x) const                  { return Alge::isInRange(x,min,max) ? 1. / N() : 0;  }
    virtual Real cdf(Real x) const                  { if (x < min) return 0; return x > max ? Real(1) : (Alge::floor(x) - min + 1) / N(); }
    virtual Real cdfInv(Real P) const               { if (P < 0) return min-1; return P > 1 ? max : Alge::floor(min + (P+Real_::zeroTol)*N() - 1); }
    virtual Real mean() const                       { return 0.5*(min+max); }
    virtual Real variance() const                   { return (Alge::sqr(N()) - 1) / 12; }

    /// Same as next() but returns an integer rather than a real
    Int nextInt() const                             { return _std ? nextStd(getGen()) : (Alge::abs(nextStd(getGen())) % (max-min+1)) + min;  }

private:
    Real N() const                                  { return Real(max)-min+1; }
    bool _std;
public:
    Int min;
    Int max;
};

typedef Discrete_<int32> Discrete;
typedef Discrete_<int64> Discrete_d;

extern template class Discrete_<int32>;
extern template class Discrete_<int64>;

}
