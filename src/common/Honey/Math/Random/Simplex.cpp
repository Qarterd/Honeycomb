// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Simplex.h"
#include "Honey/Math/Random/Dist/Discrete.h"

//noise() functions based on sdnoise1234.c by Stefan Gustavson

namespace honey
{

template<class Subclass, int Dim, class Real>
SimplexNoiseCommon<Subclass,Dim,Real>::SimplexNoiseCommon(RandomGen& gen, int size) :
    _size(size)
{
    //Generate a permutation table of random indices from [0, size)
    Discrete disc(gen, 0, _size-1);
    for (auto i: range(_size)) { _perm.push_back(disc.next()); mt_unused(i); }
    //Permutation table is repeated twice to avoid needing to wrap the index for each lookup
    for (auto i: range(_size)) _perm.push_back(_perm[i]);
}

template<class Real>
void SimplexNoise<1,Real>::grad(int hash, Real& gx)
{
    int h = hash & 15;
    gx = 1 + (h & 7);    // Gradient value is one of 1.0, 2.0, ..., 8.0
    if (h&8) gx = -gx;   // Make half of the gradients negative
}

template<class Real>
Real SimplexNoise<1,Real>::noise(const Vec& x_, optional<Vec&> dx)
{
    auto x = map(x_, Vec(), [](Real e) { return Alge::abs(e); }); //mirror about origin
    int i0 = Alge::floor(x.x);
    int i1 = i0 + 1;
    Real x0 = x.x - i0;
    Real x1 = x0 - 1;

    Real gx0, gx1;
    Real n0, n1;
    Real t20, t40, t21, t41;

    Real x20 = x0*x0;
    Real t0 = 1 - x20;
    //  if(t0 < 0) t0 = 0; // Never happens for 1D: x0<=1 always
    t20 = t0 * t0;
    t40 = t20 * t20;
    grad(this->_perm[i0 % this->_size], gx0);
    n0 = t40 * gx0 * x0;

    Real x21 = x1*x1;
    Real t1 = 1 - x21;
    //  if(t1 < 0) t1 = 0; // Never happens for 1D: |x1|<=1 always
    t21 = t1 * t1;
    t41 = t21 * t21;
    grad(this->_perm[i1 % this->_size], gx1);
    n1 = t41 * gx1 * x1;

    // The maximum value of this noise is 8*(3/4)^4 = 2.53125
    static const Real scale = 1 / (8 * Alge::pow(Real(3)/4,4));
    
    if (dx)
    {
        /* Compute derivative according to:
        *  dx->x = -8 * t20 * t0 * x0 * (gx0 * x0) + t40 * gx0;
        *  dx->x += -8 * t21 * t1 * x1 * (gx1 * x1) + t41 * gx1;
        */
        dx->x = t20 * t0 * gx0 * x20;
        dx->x += t21 * t1 * gx1 * x21;
        *dx *= -8;
        dx->x += t40 * gx0 + t41 * gx1;
        *dx *= scale; /* Scale derivative to match the noise scaling */
    }
    return scale * (n0 + n1);
}

/// Helper to compute gradient-dot-residualvector
template<class Real>
void SimplexNoise<2,Real>::grad(int hash, Real& gx, Real& gy)
{
    /*
     * Gradient tables. These could be programmed the Ken Perlin way with
     * some clever bit-twiddling, but this is more clear, and not really slower.
     */
    static const Real lut[8][2] =
    {
        { -1.0, -1.0 }, { 1.0, 0.0 } , { -1.0, 0.0 } , { 1.0, 1.0 } ,
        { -1.0, 1.0 } , { 0.0, -1.0 } , { 0.0, 1.0 } , { 1.0, -1.0 }
    };

    int h = hash & 7;
    gx = lut[h][0];
    gy = lut[h][1];
}

template<class Real>
Real SimplexNoise<2,Real>::noise(const Vec& x_, optional<Vec&> dx)
{
    auto x = map(x_, Vec(), [](Real e) { return Alge::abs(e); }); //mirror about origin
    
    Real n0, n1, n2; /* Noise contributions from the three simplex corners */
    Real gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */

    // Skewing factors for 2D simplex grid
    static const Real F2 = Real(0.5)*(Alge::sqrt(3) - 1); //0.366025403
    static const Real G2 = (3 - Alge::sqrt(3)) / 6; //0.211324865

    Real s = ( x.x + x.y ) * F2; /* Hairy factor for 2D */
    Real xs = x.x + s;
    Real ys = x.y + s;
    int i = Alge::floor( xs );
    int j = Alge::floor( ys );

    Real t = ( Real ) ( i + j ) * G2;
    Real X0 = i - t; /* Unskew the cell origin back to (x,y) space */
    Real Y0 = j - t;
    Real x0 = x.x - X0; /* The x,y distances from the cell origin */
    Real y0 = x.y - Y0;

    /* For the 2D case, the simplex shape is an equilateral triangle.
     * Determine which simplex we are in. */
    int i1, j1; /* Offsets for second (middle) corner of simplex in (i,j) coords */
    if (x0 > y0) { i1 = 1; j1 = 0; } /* lower triangle, XY order: (0,0)->(1,0)->(1,1) */
    else { i1 = 0; j1 = 1; }      /* upper triangle, YX order: (0,0)->(0,1)->(1,1) */

    /* A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
     * a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
     * c = (3-sqrt(3))/6   */
    Real x1 = x0 - i1 + G2; /* Offsets for middle corner in (x,y) unskewed coords */
    Real y1 = y0 - j1 + G2;
    Real x2 = x0 - 1 + 2 * G2; /* Offsets for last corner in (x,y) unskewed coords */
    Real y2 = y0 - 1 + 2 * G2;

    /* Wrap the integer indices, to avoid indexing perm[] out of bounds */
    int ii = i % this->_size;
    int jj = j % this->_size;

    /* Calculate the contribution from the three corners */
    Real t0 = Real(0.5) - x0 * x0 - y0 * y0;
    Real t20, t40;
    if (t0 < 0) t40 = t20 = t0 = n0 = gx0 = gy0 = 0; /* No influence */
    else
    {
        grad( this->_perm[ii + this->_perm[jj]], gx0, gy0 );
        t20 = t0 * t0;
        t40 = t20 * t20;
        n0 = t40 * ( gx0 * x0 + gy0 * y0 );
    }

    Real t1 = Real(0.5) - x1 * x1 - y1 * y1;
    Real t21, t41;
    if (t1 < 0) t21 = t41 = t1 = n1 = gx1 = gy1 = 0; /* No influence */
    else
    {
        grad( this->_perm[ii + i1 + this->_perm[jj + j1]], gx1, gy1 );
        t21 = t1 * t1;
        t41 = t21 * t21;
        n1 = t41 * ( gx1 * x1 + gy1 * y1 );
    }

    Real t2 = Real(0.5) - x2 * x2 - y2 * y2;
    Real t22, t42;
    if (t2 < 0) t42 = t22 = t2 = n2 = gx2 = gy2 = 0; /* No influence */
    else
    {
        grad( this->_perm[ii + 1 + this->_perm[jj + 1]], gx2, gy2 );
        t22 = t2 * t2;
        t42 = t22 * t22;
        n2 = t42 * ( gx2 * x2 + gy2 * y2 );
    }

    /* Add contributions from each corner to get the final noise value.
     * The result is scaled to return values in the interval [-1,1]. */
    static const Real scale = 70;
    Real noise = scale * ( n0 + n1 + n2 );

    // Compute derivative, if requested
    if (dx)
    {
        /*  A straight, unoptimised calculation would be like:
         *    dx->x = -8 * t20 * t0 * x0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gx0;
         *    dx->y = -8 * t20 * t0 * y0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gy0;
         *    dx->x += -8 * t21 * t1 * x1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gx1;
         *    dx->y += -8 * t21 * t1 * y1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gy1;
         *    dx->x += -8 * t22 * t2 * x2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gx2;
         *    dx->y += -8 * t22 * t2 * y2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gy2;
         */
        Real temp0 = t20 * t0 * ( gx0* x0 + gy0 * y0 );
        dx->x = temp0 * x0;
        dx->y = temp0 * y0;
        Real temp1 = t21 * t1 * ( gx1 * x1 + gy1 * y1 );
        dx->x += temp1 * x1;
        dx->y += temp1 * y1;
        Real temp2 = t22 * t2 * ( gx2* x2 + gy2 * y2 );
        dx->x += temp2 * x2;
        dx->y += temp2 * y2;
        *dx *= -8;
        dx->x += t40 * gx0 + t41 * gx1 + t42 * gx2;
        dx->y += t40 * gy0 + t41 * gy1 + t42 * gy2;
        *dx *= scale; /* Scale derivative to match the noise scaling */
    }
    return noise;
}

template<class Real>
void SimplexNoise<3,Real>::grad(int hash, Real& gx, Real& gy, Real& gz)
{
    /*
     * Gradient directions for 3D.
     * These vectors are based on the midpoints of the 12 edges of a cube.
     * A larger array of random unit length vectors would also do the job,
     * but these 12 (including 4 repeats to make the array length a power
     * of two) work better. They are not random, they are carefully chosen
     * to represent a small, isotropic set of directions.
     */
    static const Real lut[16][3] =
    {
        { 1.0, 0.0, 1.0 }, { 0.0, 1.0, 1.0 }, // 12 cube edges
        { -1.0, 0.0, 1.0 }, { 0.0, -1.0, 1.0 },
        { 1.0, 0.0, -1.0 }, { 0.0, 1.0, -1.0 },
        { -1.0, 0.0, -1.0 }, { 0.0, -1.0, -1.0 },
        { 1.0, -1.0, 0.0 }, { 1.0, 1.0, 0.0 },
        { -1.0, 1.0, 0.0 }, { -1.0, -1.0, 0.0 },
        { 1.0, 0.0, 1.0 }, { -1.0, 0.0, 1.0 }, // 4 repeats to make 16
        { 0.0, 1.0, -1.0 }, { 0.0, -1.0, -1.0 }
    };

    int h = hash & 15;
    gx = lut[h][0];
    gy = lut[h][1];
    gz = lut[h][2];
}

template<class Real>
Real SimplexNoise<3,Real>::noise(const Vec& x_, optional<Vec&> dx)
{
    auto x = map(x_, Vec(), [](Real e) { return Alge::abs(e); }); //mirror about origin
    
    Real n0, n1, n2, n3; /* Noise contributions from the four simplex corners */
    Real noise;          /* Return value */
    Real gx0, gy0, gz0, gx1, gy1, gz1; /* Gradients at simplex corners */
    Real gx2, gy2, gz2, gx3, gy3, gz3;

    // Skewing factors for 3D simplex grid
    static const Real F3 = Real(1) / 3;
    static const Real G3 = Real(1) / 6;
    
    /* Skew the input space to determine which simplex cell we're in */
    Real s = (x.x+x.y+x.z)*F3; /* Very nice and simple skew factor for 3D */
    Real xs = x.x+s;
    Real ys = x.y+s;
    Real zs = x.z+s;
    int i = Alge::floor(xs);
    int j = Alge::floor(ys);
    int k = Alge::floor(zs);

    Real t = (Real)(i+j+k)*G3; 
    Real X0 = i-t; /* Unskew the cell origin back to (x,y,z) space */
    Real Y0 = j-t;
    Real Z0 = k-t;
    Real x0 = x.x-X0; /* The x,y,z distances from the cell origin */
    Real y0 = x.y-Y0;
    Real z0 = x.z-Z0;

    /* For the 3D case, the simplex shape is a slightly irregular tetrahedron.
     * Determine which simplex we are in. */
    int i1, j1, k1; /* Offsets for second corner of simplex in (i,j,k) coords */
    int i2, j2, k2; /* Offsets for third corner of simplex in (i,j,k) coords */

    if (x0>=y0)
    {
        if (y0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } /* X Y Z order */
        else if (x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } /* X Z Y order */
        else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } /* Z X Y order */
    }
    else // x0<y0
    {
        if (y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } /* Z Y X order */
        else if (x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } /* Y Z X order */
        else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } /* Y X Z order */
    }

    /* A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
     * a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
     * a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
     * c = 1/6.   */

    Real x1 = x0 - i1 + G3; /* Offsets for second corner in (x,y,z) coords */
    Real y1 = y0 - j1 + G3;
    Real z1 = z0 - k1 + G3;
    Real x2 = x0 - i2 + 2 * G3; /* Offsets for third corner in (x,y,z) coords */
    Real y2 = y0 - j2 + 2 * G3;
    Real z2 = z0 - k2 + 2 * G3;
    Real x3 = x0 - 1 + 3 * G3; /* Offsets for last corner in (x,y,z) coords */
    Real y3 = y0 - 1 + 3 * G3;
    Real z3 = z0 - 1 + 3 * G3;

    /* Wrap the integer indices, to avoid indexing perm[] out of bounds */
    int ii = i % this->_size;
    int jj = j % this->_size;
    int kk = k % this->_size;

    /* Calculate the contribution from the four corners */
    Real t0 = Real(0.6) - x0*x0 - y0*y0 - z0*z0;
    Real t20, t40;
    if (t0 < 0) n0 = t0 = t20 = t40 = gx0 = gy0 = gz0 = 0;
    else
    {
        grad( this->_perm[ii + this->_perm[jj + this->_perm[kk]]], gx0, gy0, gz0 );
        t20 = t0 * t0;
        t40 = t20 * t20;
        n0 = t40 * ( gx0 * x0 + gy0 * y0 + gz0 * z0 );
    }

    Real t1 = Real(0.6) - x1*x1 - y1*y1 - z1*z1;
    Real t21, t41;
    if (t1 < 0) n1 = t1 = t21 = t41 = gx1 = gy1 = gz1 = 0;
    else
    {
        grad( this->_perm[ii + i1 + this->_perm[jj + j1 + this->_perm[kk + k1]]], gx1, gy1, gz1 );
        t21 = t1 * t1;
        t41 = t21 * t21;
        n1 = t41 * ( gx1 * x1 + gy1 * y1 + gz1 * z1 );
    }

    Real t2 = Real(0.6) - x2*x2 - y2*y2 - z2*z2;
    Real t22, t42;
    if (t2 < 0) n2 = t2 = t22 = t42 = gx2 = gy2 = gz2 = 0;
    else
    {
        grad( this->_perm[ii + i2 + this->_perm[jj + j2 + this->_perm[kk + k2]]], gx2, gy2, gz2 );
        t22 = t2 * t2;
        t42 = t22 * t22;
        n2 = t42 * ( gx2 * x2 + gy2 * y2 + gz2 * z2 );
    }

    Real t3 = Real(0.6) - x3*x3 - y3*y3 - z3*z3;
    Real t23, t43;
    if (t3 < 0) n3 = t3 = t23 = t43 = gx3 = gy3 = gz3 = 0;
    else
    {
        grad( this->_perm[ii + 1 + this->_perm[jj + 1 + this->_perm[kk + 1]]], gx3, gy3, gz3 );
        t23 = t3 * t3;
        t43 = t23 * t23;
        n3 = t43 * ( gx3 * x3 + gy3 * y3 + gz3 * z3 );
    }

    /* Add contributions from each corner to get the final noise value.
     * The result is scaled to return values in the range [-1,1] */
    static const Real scale = 32;
    noise = scale * (n0 + n1 + n2 + n3);

    // Compute derivative, if requested
    if (dx)
    {
        /*  A straight, unoptimised calculation would be like:
         *  dx->x = -8 * t20 * t0 * x0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gx0;
         *  dx->y = -8 * t20 * t0 * y0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gy0;
         *  dx->z = -8 * t20 * t0 * z0 * dot(gx0, gy0, gz0, x0, y0, z0) + t40 * gz0;
         *  dx->x += -8 * t21 * t1 * x1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gx1;
         *  dx->y += -8 * t21 * t1 * y1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gy1;
         *  dx->z += -8 * t21 * t1 * z1 * dot(gx1, gy1, gz1, x1, y1, z1) + t41 * gz1;
         *  dx->x += -8 * t22 * t2 * x2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gx2;
         *  dx->y += -8 * t22 * t2 * y2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gy2;
         *  dx->z += -8 * t22 * t2 * z2 * dot(gx2, gy2, gz2, x2, y2, z2) + t42 * gz2;
         *  dx->x += -8 * t23 * t3 * x3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gx3;
         *  dx->y += -8 * t23 * t3 * y3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gy3;
         *  dx->z += -8 * t23 * t3 * z3 * dot(gx3, gy3, gz3, x3, y3, z3) + t43 * gz3;
         */
        Real temp0 = t20 * t0 * ( gx0 * x0 + gy0 * y0 + gz0 * z0 );
        dx->x = temp0 * x0;
        dx->y = temp0 * y0;
        dx->z = temp0 * z0;
        Real temp1 = t21 * t1 * ( gx1 * x1 + gy1 * y1 + gz1 * z1 );
        dx->x += temp1 * x1;
        dx->y += temp1 * y1;
        dx->z += temp1 * z1;
        Real temp2 = t22 * t2 * ( gx2 * x2 + gy2 * y2 + gz2 * z2 );
        dx->x += temp2 * x2;
        dx->y += temp2 * y2;
        dx->z += temp2 * z2;
        Real temp3 = t23 * t3 * ( gx3 * x3 + gy3 * y3 + gz3 * z3 );
        dx->x += temp3 * x3;
        dx->y += temp3 * y3;
        dx->z += temp3 * z3;
        *dx *= -8;
        dx->x += t40 * gx0 + t41 * gx1 + t42 * gx2 + t43 * gx3;
        dx->y += t40 * gy0 + t41 * gy1 + t42 * gy2 + t43 * gy3;
        dx->z += t40 * gz0 + t41 * gz1 + t42 * gz2 + t43 * gz3;
        *dx *= scale; /* Scale derivative to match the noise scaling */
    }
    return noise;
}

template<class Real>
void SimplexNoise<4,Real>::grad(int hash, Real& gx, Real& gy, Real& gz, Real& gw)
{
    static const Real lut[32][4] =
    {
        { 0.0, 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0, -1.0 }, { 0.0, 1.0, -1.0, 1.0 }, { 0.0, 1.0, -1.0, -1.0 }, // 32 tesseract edges
        { 0.0, -1.0, 1.0, 1.0 }, { 0.0, -1.0, 1.0, -1.0 }, { 0.0, -1.0, -1.0, 1.0 }, { 0.0, -1.0, -1.0, -1.0 },
        { 1.0, 0.0, 1.0, 1.0 }, { 1.0, 0.0, 1.0, -1.0 }, { 1.0, 0.0, -1.0, 1.0 }, { 1.0, 0.0, -1.0, -1.0 },
        { -1.0, 0.0, 1.0, 1.0 }, { -1.0, 0.0, 1.0, -1.0 }, { -1.0, 0.0, -1.0, 1.0 }, { -1.0, 0.0, -1.0, -1.0 },
        { 1.0, 1.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0, -1.0 }, { 1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, -1.0 },
        { -1.0, 1.0, 0.0, 1.0 }, { -1.0, 1.0, 0.0, -1.0 }, { -1.0, -1.0, 0.0, 1.0 }, { -1.0, -1.0, 0.0, -1.0 },
        { 1.0, 1.0, 1.0, 0.0 }, { 1.0, 1.0, -1.0, 0.0 }, { 1.0, -1.0, 1.0, 0.0 }, { 1.0, -1.0, -1.0, 0.0 },
        { -1.0, 1.0, 1.0, 0.0 }, { -1.0, 1.0, -1.0, 0.0 }, { -1.0, -1.0, 1.0, 0.0 }, { -1.0, -1.0, -1.0, 0.0 }
    };

    int h = hash & 31;
    gx = lut[h][0];
    gy = lut[h][1];
    gz = lut[h][2];
    gw = lut[h][3];
}

template<class Real>
Real SimplexNoise<4,Real>::noise(const Vec& x_, optional<Vec&> dx)
{
    auto x = map(x_, Vec(), [](Real e) { return Alge::abs(e); }); //mirror about origin
    
    Real n0, n1, n2, n3, n4; // Noise contributions from the five corners
    Real noise; // Return value
    Real gx0, gy0, gz0, gw0, gx1, gy1, gz1, gw1; /* Gradients at simplex corners */
    Real gx2, gy2, gz2, gw2, gx3, gy3, gz3, gw3, gx4, gy4, gz4, gw4;
    Real t20, t21, t22, t23, t24;
    Real t40, t41, t42, t43, t44;

    // The skewing and unskewing factors are hairy again for the 4D case
    static const Real F4 = (Alge::sqrt(5) - 1) / 4; //0.309016994
    static const Real G4 = (5 - Alge::sqrt(5)) / 20; //0.138196601
    
    // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
    Real s = (x.x + x.y + x.z + x.w) * F4; // Factor for 4D skewing
    Real xs = x.x + s;
    Real ys = x.y + s;
    Real zs = x.z + s;
    Real ws = x.w + s;
    int i = Alge::floor(xs);
    int j = Alge::floor(ys);
    int k = Alge::floor(zs);
    int l = Alge::floor(ws);

    Real t = (i + j + k + l) * G4; // Factor for 4D unskewing
    Real X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
    Real Y0 = j - t;
    Real Z0 = k - t;
    Real W0 = l - t;

    Real x0 = x.x - X0;  // The x,y,z,w distances from the cell origin
    Real y0 = x.y - Y0;
    Real z0 = x.z - Z0;
    Real w0 = x.w - W0;

    // For the 4D case, the simplex is a 4D shape I won't even try to describe.
    // To find out which of the 24 possible simplices we're in, we need to
    // determine the magnitude ordering of x0, y0, z0 and w0.
    // The method below is a reasonable way of finding the ordering of x,y,z,w
    // and then find the correct traversal order for the simplex weíre in.
    // First, six pair-wise comparisons are performed between each possible pair
    // of the four coordinates, and then the results are used to add up binary
    // bits for an integer index into a precomputed lookup table, simplex[].
    int c1 = (x0 > y0) ? 32 : 0;
    int c2 = (x0 > z0) ? 16 : 0;
    int c3 = (y0 > z0) ? 8 : 0;
    int c4 = (x0 > w0) ? 4 : 0;
    int c5 = (y0 > w0) ? 2 : 0;
    int c6 = (z0 > w0) ? 1 : 0;
    int c = c1 | c2 | c3 | c4 | c5 | c6; // '|' is mostly faster than '+'

    int i1, j1, k1, l1; // The integer offsets for the second simplex corner
    int i2, j2, k2, l2; // The integer offsets for the third simplex corner
    int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner

    // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
    // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
    // impossible. Only the 24 indices which have non-zero entries make any sense.
    // We use a thresholding to set the coordinates in turn from the largest magnitude.
    // The number 3 in the "simplex" array is at the position of the largest coordinate.
    static const unsigned char simplex[64][4] =
    {
        {0,1,2,3},{0,1,3,2},{0,0,0,0},{0,2,3,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,2,3,0},
        {0,2,1,3},{0,0,0,0},{0,3,1,2},{0,3,2,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,3,2,0},
        {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
        {1,2,0,3},{0,0,0,0},{1,3,0,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,3,0,1},{2,3,1,0},
        {1,0,2,3},{1,0,3,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,0,3,1},{0,0,0,0},{2,1,3,0},
        {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
        {2,0,1,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,0,1,2},{3,0,2,1},{0,0,0,0},{3,1,2,0},
        {2,1,0,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,1,0,2},{0,0,0,0},{3,2,0,1},{3,2,1,0}
    };
  
    i1 = simplex[c][0]>=3 ? 1 : 0;
    j1 = simplex[c][1]>=3 ? 1 : 0;
    k1 = simplex[c][2]>=3 ? 1 : 0;
    l1 = simplex[c][3]>=3 ? 1 : 0;
    // The number 2 in the "simplex" array is at the second largest coordinate.
    i2 = simplex[c][0]>=2 ? 1 : 0;
    j2 = simplex[c][1]>=2 ? 1 : 0;
    k2 = simplex[c][2]>=2 ? 1 : 0;
    l2 = simplex[c][3]>=2 ? 1 : 0;
    // The number 1 in the "simplex" array is at the second smallest coordinate.
    i3 = simplex[c][0]>=1 ? 1 : 0;
    j3 = simplex[c][1]>=1 ? 1 : 0;
    k3 = simplex[c][2]>=1 ? 1 : 0;
    l3 = simplex[c][3]>=1 ? 1 : 0;
    // The fifth corner has all coordinate offsets = 1, so no need to look that up.

    Real x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
    Real y1 = y0 - j1 + G4;
    Real z1 = z0 - k1 + G4;
    Real w1 = w0 - l1 + G4;
    Real x2 = x0 - i2 + 2 * G4; // Offsets for third corner in (x,y,z,w) coords
    Real y2 = y0 - j2 + 2 * G4;
    Real z2 = z0 - k2 + 2 * G4;
    Real w2 = w0 - l2 + 2 * G4;
    Real x3 = x0 - i3 + 3 * G4; // Offsets for fourth corner in (x,y,z,w) coords
    Real y3 = y0 - j3 + 3 * G4;
    Real z3 = z0 - k3 + 3 * G4;
    Real w3 = w0 - l3 + 3 * G4;
    Real x4 = x0 - 1 + 4 * G4; // Offsets for last corner in (x,y,z,w) coords
    Real y4 = y0 - 1 + 4 * G4;
    Real z4 = z0 - 1 + 4 * G4;
    Real w4 = w0 - 1 + 4 * G4;

    // Wrap the integer indices, to avoid indexing perm[] out of bounds
    int ii = i % this->_size;
    int jj = j % this->_size;
    int kk = k % this->_size;
    int ll = l % this->_size;

    // Calculate the contribution from the five corners
    Real t0 = Real(0.6) - x0*x0 - y0*y0 - z0*z0 - w0*w0;
    if (t0 < 0) n0 = t0 = t20 = t40 = gx0 = gy0 = gz0 = gw0 = 0;
    else
    {
        t20 = t0 * t0;
        t40 = t20 * t20;
        grad(this->_perm[ii+this->_perm[jj+this->_perm[kk+this->_perm[ll]]]], gx0, gy0, gz0, gw0);
        n0 = t40 * ( gx0 * x0 + gy0 * y0 + gz0 * z0 + gw0 * w0 );
    }

    Real t1 = Real(0.6) - x1*x1 - y1*y1 - z1*z1 - w1*w1;
    if (t1 < 0) n1 = t1 = t21 = t41 = gx1 = gy1 = gz1 = gw1 = 0;
    else
    {
        t21 = t1 * t1;
        t41 = t21 * t21;
        grad(this->_perm[ii+i1+this->_perm[jj+j1+this->_perm[kk+k1+this->_perm[ll+l1]]]], gx1, gy1, gz1, gw1);
        n1 = t41 * ( gx1 * x1 + gy1 * y1 + gz1 * z1 + gw1 * w1 );
    }

    Real t2 = Real(0.6) - x2*x2 - y2*y2 - z2*z2 - w2*w2;
    if (t2 < 0) n2 = t2 = t22 = t42 = gx2 = gy2 = gz2 = gw2 = 0;
    else
    {
        t22 = t2 * t2;
        t42 = t22 * t22;
        grad(this->_perm[ii+i2+this->_perm[jj+j2+this->_perm[kk+k2+this->_perm[ll+l2]]]], gx2, gy2, gz2, gw2);
        n2 = t42 * ( gx2 * x2 + gy2 * y2 + gz2 * z2 + gw2 * w2 );
    }

    Real t3 = Real(0.6) - x3*x3 - y3*y3 - z3*z3 - w3*w3;
    if (t3 < 0) n3 = t3 = t23 = t43 = gx3 = gy3 = gz3 = gw3 = 0;
    else
    {
        t23 = t3 * t3;
        t43 = t23 * t23;
        grad(this->_perm[ii+i3+this->_perm[jj+j3+this->_perm[kk+k3+this->_perm[ll+l3]]]], gx3, gy3, gz3, gw3);
        n3 = t43 * ( gx3 * x3 + gy3 * y3 + gz3 * z3 + gw3 * w3 );
    }

    Real t4 = Real(0.6) - x4*x4 - y4*y4 - z4*z4 - w4*w4;
    if (t4 < 0) n4 = t4 = t24 = t44 = gx4 = gy4 = gz4 = gw4 = 0;
    else
    {
        t24 = t4 * t4;
        t44 = t24 * t24;
        grad(this->_perm[ii+1+this->_perm[jj+1+this->_perm[kk+1+this->_perm[ll+1]]]], gx4, gy4, gz4, gw4);
        n4 = t44 * ( gx4 * x4 + gy4 * y4 + gz4 * z4 + gw4 * w4 );
    }

    // Sum up and scale the result to cover the range [-1,1]
    static const Real scale = 27;
    noise = scale * (n0 + n1 + n2 + n3 + n4);

    // Compute derivative, if requested
    if (dx)
    {
        /*  A straight, unoptimised calculation would be like:
         *  dx->x = -8 * t20 * t0 * x0 * dot(gx0, gy0, gz0, gw0, x0, y0, z0, w0) + t40 * gx0;
         *  dx->y = -8 * t20 * t0 * y0 * dot(gx0, gy0, gz0, gw0, x0, y0, z0, w0) + t40 * gy0;
         *  dx->z = -8 * t20 * t0 * z0 * dot(gx0, gy0, gz0, gw0, x0, y0, z0, w0) + t40 * gz0;
         *  dx->w = -8 * t20 * t0 * w0 * dot(gx0, gy0, gz0, gw0, x0, y0, z0, w0) + t40 * gw0;
         *  dx->x += -8 * t21 * t1 * x1 * dot(gx1, gy1, gz1, gw1, x1, y1, z1, w1) + t41 * gx1;
         *  dx->y += -8 * t21 * t1 * y1 * dot(gx1, gy1, gz1, gw1, x1, y1, z1, w1) + t41 * gy1;
         *  dx->z += -8 * t21 * t1 * z1 * dot(gx1, gy1, gz1, gw1, x1, y1, z1, w1) + t41 * gz1;
         *  dx->w = -8 * t21 * t1 * w1 * dot(gx1, gy1, gz1, gw1, x1, y1, z1, w1) + t41 * gw1;
         *  dx->x += -8 * t22 * t2 * x2 * dot(gx2, gy2, gz2, gw2, x2, y2, z2, w2) + t42 * gx2;
         *  dx->y += -8 * t22 * t2 * y2 * dot(gx2, gy2, gz2, gw2, x2, y2, z2, w2) + t42 * gy2;
         *  dx->z += -8 * t22 * t2 * z2 * dot(gx2, gy2, gz2, gw2, x2, y2, z2, w2) + t42 * gz2;
         *  dx->w += -8 * t22 * t2 * w2 * dot(gx2, gy2, gz2, gw2, x2, y2, z2, w2) + t42 * gw2;
         *  dx->x += -8 * t23 * t3 * x3 * dot(gx3, gy3, gz3, gw3, x3, y3, z3, w3) + t43 * gx3;
         *  dx->y += -8 * t23 * t3 * y3 * dot(gx3, gy3, gz3, gw3, x3, y3, z3, w3) + t43 * gy3;
         *  dx->z += -8 * t23 * t3 * z3 * dot(gx3, gy3, gz3, gw3, x3, y3, z3, w3) + t43 * gz3;
         *  dx->w += -8 * t23 * t3 * w3 * dot(gx3, gy3, gz3, gw3, x3, y3, z3, w3) + t43 * gw3;
         *  dx->x += -8 * t24 * t4 * x4 * dot(gx4, gy4, gz4, gw4, x4, y4, z4, w4) + t44 * gx4;
         *  dx->y += -8 * t24 * t4 * y4 * dot(gx4, gy4, gz4, gw4, x4, y4, z4, w4) + t44 * gy4;
         *  dx->z += -8 * t24 * t4 * z4 * dot(gx4, gy4, gz4, gw4, x4, y4, z4, w4) + t44 * gz4;
         *  dx->w += -8 * t24 * t4 * w4 * dot(gx4, gy4, gz4, gw4, x4, y4, z4, w4) + t44 * gw4;
         */
        Real temp0 = t20 * t0 * ( gx0 * x0 + gy0 * y0 + gz0 * z0 + gw0 * w0 );
        dx->x = temp0 * x0;
        dx->y = temp0 * y0;
        dx->z = temp0 * z0;
        dx->w = temp0 * w0;
        Real temp1 = t21 * t1 * ( gx1 * x1 + gy1 * y1 + gz1 * z1 + gw1 * w1 );
        dx->x += temp1 * x1;
        dx->y += temp1 * y1;
        dx->z += temp1 * z1;
        dx->w += temp1 * w1;
        Real temp2 = t22 * t2 * ( gx2 * x2 + gy2 * y2 + gz2 * z2 + gw2 * w2 );
        dx->x += temp2 * x2;
        dx->y += temp2 * y2;
        dx->z += temp2 * z2;
        dx->w += temp2 * w2;
        Real temp3 = t23 * t3 * ( gx3 * x3 + gy3 * y3 + gz3 * z3 + gw3 * w3 );
        dx->x += temp3 * x3;
        dx->y += temp3 * y3;
        dx->z += temp3 * z3;
        dx->w += temp3 * w3;
        Real temp4 = t24 * t4 * ( gx4 * x4 + gy4 * y4 + gz4 * z4 + gw4 * w4 );
        dx->x += temp4 * x4;
        dx->y += temp4 * y4;
        dx->z += temp4 * z4;
        dx->w += temp4 * w4;
        *dx *= -8;
        dx->x += t40 * gx0 + t41 * gx1 + t42 * gx2 + t43 * gx3 + t44 * gx4;
        dx->y += t40 * gy0 + t41 * gy1 + t42 * gy2 + t43 * gy3 + t44 * gy4;
        dx->z += t40 * gz0 + t41 * gz1 + t42 * gz2 + t43 * gz3 + t44 * gz4;
        dx->w += t40 * gw0 + t41 * gw1 + t42 * gw2 + t43 * gw3 + t44 * gw4;
        *dx *= scale; /* Scale derivative to match the noise scaling */
    }
    return noise;
}

template class SimplexNoise<1, Float>;
template class SimplexNoise<2, Float>;
template class SimplexNoise<3, Float>;
template class SimplexNoise<4, Float>;

template class SimplexNoise<1, Double>;
template class SimplexNoise<2, Double>;
template class SimplexNoise<3, Double>;
template class SimplexNoise<4, Double>;

}
