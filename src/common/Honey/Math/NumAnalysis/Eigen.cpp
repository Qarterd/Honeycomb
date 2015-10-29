// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/NumAnalysis/Eigen.h"

namespace honey
{

template<class Real>
void Eigen<Real>::jacobi(Matrix& A, Vec& W, optional<Matrix&> V, vector<sdt>& indR, vector<sdt>& indC)
{
    const Real eps = Real_::epsilon;
    sdt i, j, k, m;
    sdt n = A.rows();

    if (V) V->fromIdentity();
    
    sdt iters, maxIters = n*n*30;
    Real mv = (Real)0;
    
    for( k = 0; k < n; k++ )
    {
        W[k] = A(k,k);
        if( k < n - 1 )
        {
            for( m = k+1, mv = Alge::abs(A(k,m)), i = k+2; i < n; i++ )
            {
                Real val = Alge::abs(A(k,i));
                if( mv < val )
                    mv = val, m = i;
            }
            indR[k] = m;
        }
        if( k > 0 )
        {
            for( m = 0, mv = Alge::abs(A(k)), i = 1; i < k; i++ )
            {
                Real val = Alge::abs(A(i,k));
                if( mv < val )
                    mv = val, m = i;
            }
            indC[k] = m;
        }
    }
    
    if( n > 1 ) for( iters = 0; iters < maxIters; iters++ )
    {
        // find index (k,l) of pivot p
        for( k = 0, mv = Alge::abs(A(indR[0])), i = 1; i < n-1; i++ )
        {
            Real val = Alge::abs(A(i,indR[i]));
            if( mv < val )
                mv = val, k = i;
        }
        sdt l = indR[k];
        for( i = 1; i < n; i++ )
        {
            Real val = Alge::abs(A(indC[i],i));
            if( mv < val )
                mv = val, k = indC[i], l = i;
        }
        
        Real p = A(k,l);
        if( Alge::abs(p) <= eps )
            break;
        Real y = (Real)((W[l] - W[k])*0.5);
        Real t = Alge::abs(y) + Alge::hypot(p, y);
        Real s = Alge::hypot(p, t);
        Real c = t/s;
        s = p/s; t = (p/t)*p;
        if( y < 0 )
            s = -s, t = -t;
        A(k,l) = 0;
        
        W[k] -= t;
        W[l] += t;
        
        Real a0, b0;
        
#undef rotate
#define rotate(v0, v1) a0 = v0, b0 = v1, v0 = a0*c - b0*s, v1 = a0*s + b0*c
        
        // rotate rows and columns k and l
        for( i = 0; i < k; i++ )
            rotate(A(i,k), A(i,l));
        for( i = k+1; i < l; i++ )
            rotate(A(k,i), A(i,l));
        for( i = l+1; i < n; i++ )
            rotate(A(k,i), A(l,i));
        
        // rotate eigenvectors
        if( V )
            for( i = 0; i < n; i++ )
                rotate((*V)(i,k), (*V)(i,l));
        
#undef rotate
        
        for( j = 0; j < 2; j++ )
        {
            sdt idx = j == 0 ? k : l;
            if( idx < n - 1 )
            {
                for( m = idx+1, mv = Alge::abs(A(idx,m)), i = idx+2; i < n; i++ )
                {
                    Real val = Alge::abs(A(idx,i));
                    if( mv < val )
                        mv = val, m = i;
                }
                indR[idx] = m;
            }
            if( idx > 0 )
            {
                for( m = 0, mv = Alge::abs(A(idx)), i = 1; i < idx; i++ )
                {
                    Real val = Alge::abs(A(i,idx));
                    if( mv < val )
                        mv = val, m = i;
                }
                indC[idx] = m;
            }
        }
    }
    
    // sort eigenvalues & eigenvectors
    for( k = 0; k < n-1; k++ )
    {
        m = k;
        for( i = k+1; i < n; i++ )
        {
            if( W[m] < W[i] )
                m = i;
        }
        if( k != m )
        {
            std::swap(W[m], W[k]);
            if( V )
                for( i = 0; i < n; i++ )
                    std::swap((*V)(i,m), (*V)(i,k));
        }
    }
}

template class Eigen<Float>;
template class Eigen<Double>;
template class Eigen<Quad>;

}
