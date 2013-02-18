// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Alge/Alge.h"
#include "Honey/Math/NumAnalysis/Svd.h"

namespace honey
{

template<class Real>
void Svd<Real>::jacobi(Matrix& At, Vec& w, Vec_d& wd, optional<Matrix&> Vt, RandomGen& rand)
{
    int m = At.cols(), n = w.rows(), n1 = At.rows();
    Double eps = Double_::epsilon*10;
    int i, j, k, iter, max_iter = std::max(m, 30);
    int acols = At.cols(), vcols = Vt ? Vt->cols() : 0;
    Real c, s;
    Double sd;

    for( i = 0; i < n; i++ )
    {
        for( k = 0, sd = 0; k < m; k++ )
        {
            Real t = At(i,k);
            sd += (Double)t*t;
        }
        wd[i] = sd;
    }
    
    if (Vt) Vt->fromIdentity();

    for( iter = 0; iter < max_iter; iter++ )
    {
        bool changed = false;
        
        for( i = 0; i < n-1; i++ )
            for( j = i+1; j < n; j++ )
            {
                int Ai = i*acols, Aj = j*acols;
                Double a = wd[i], p = 0, b = wd[j];
                
                for( k = 0; k < m; k++ )
                    p += (Double)At(Ai+k)*At(Aj+k);
                
                if( Alge_d::abs(p) <= eps*Alge_d::sqrt((Double)a*b) )
                    continue;           
                
                p *= 2;
                Double beta = a - b, gamma = Alge_d::hypot((Double)p, beta), delta;
                if( beta < 0 )
                {
                    delta = (gamma - beta)*0.5;
                    s = (Real)Alge_d::sqrt(delta/gamma);
                    c = (Real)(p/(gamma*s*2));
                }
                else
                {
                    c = (Real)Alge_d::sqrt((gamma + beta)/(gamma*2));
                    s = (Real)(p/(gamma*c*2));
                    delta = p*p*0.5/(gamma + beta);
                }
                
                if( iter % 2 )
                {
                    wd[i] += delta;
                    wd[j] -= delta;
                    
                    for( k = 0; k < m; k++ )
                    {
                        Real t0 = c*At(Ai+k) + s*At(Aj+k);
                        Real t1 = -s*At(Ai+k) + c*At(Aj+k);
                        At(Ai+k) = t0; At(Aj+k) = t1;
                    }
                }
                else
                {
                    a = b = 0;
                    for( k = 0; k < m; k++ )
                    {
                        Real t0 = c*At(Ai+k) + s*At(Aj+k);
                        Real t1 = -s*At(Ai+k) + c*At(Aj+k);
                        At(Ai+k) = t0; At(Aj+k) = t1;
                        
                        a += (Double)t0*t0; b += (Double)t1*t1;
                    }
                    wd[i] = a; wd[j] = b;
                }
                
                changed = true;
                
                if( Vt )
                {
                    int Vi = i*vcols, Vj = j*vcols;
                    
                    for( k = 0; k < n; k++ )
                    {
                        Real t0 = c*(*Vt)(Vi+k) + s*(*Vt)(Vj+k);
                        Real t1 = -s*(*Vt)(Vi+k) + c*(*Vt)(Vj+k);
                        (*Vt)(Vi+k) = t0; (*Vt)(Vj+k) = t1;
                    }
                }
            }
        if( !changed )
            break;
    }
    
    for( i = 0; i < n; i++ )
    {
        for( k = 0, sd = 0; k < m; k++ )
        {
            Real t = At(i,k);
            sd += (Double)t*t;
        }
        wd[i] = Alge_d::sqrt(sd);
    }
    
    for( i = 0; i < n-1; i++ )
    {
        j = i;
        for( k = i+1; k < n; k++ )
        {
            if( wd[j] < wd[k] )
                j = k;
        }
        if( i != j )
        {
            std::swap(wd[i], wd[j]);
            if( Vt )
            {
                for( k = 0; k < m; k++ )
                    std::swap(At(i,k), At(j,k));
                
                for( k = 0; k < n; k++ )
                    std::swap((*Vt)(i,k), (*Vt)(j,k));
            }
        }
    }
    
    for( i = 0; i < n; i++ )
        w[i] = wd[i];

    if( !Vt )
        return;

    for( i = 0; i < n1; i++ )
    {
        sd = i < n ? wd[i] : 0;
        
        while( sd == 0 )
        {
            // if we got a zero singular value, then in order to get the corresponding left singular vector
            // we generate a random vector, project it to the previously computed left singular vectors,
            // subtract the projection and normalize the difference.
            const Real val0 = (Real)(1./m);
            for( k = 0; k < m; k++ )
            {
                Real val = (rand.next() & 256) != 0 ? val0 : -val0;
                At(i,k) = val;
            }
            for( iter = 0; iter < 2; iter++ )
            {
                for( j = 0; j < i; j++ )
                {
                    sd = 0;
                    for( k = 0; k < m; k++ )
                        sd += At(i,k)*At(j,k);
                    Real asum = 0;
                    for( k = 0; k < m; k++ )
                    {
                        Real t = (Real)(At(i,k) - sd*At(j,k));
                        At(i,k) = t;
                        asum += Alge::abs(t);
                    }
                    asum = asum ? 1/asum : 0;
                    for( k = 0; k < m; k++ )
                        At(i,k) *= asum;
                }
            }
            sd = 0;
            for( k = 0; k < m; k++ )
            {
                Real t = At(i,k);
                sd += (Double)t*t;
            }
            sd = Alge_d::sqrt(sd);
        }
        
        s = (Real)(1/sd);
        for( k = 0; k < m; k++ )
            At(i,k) *= s;
    }
}

template class Svd<Float>;
template class Svd<Double>;
template class Svd<Quad>;

}
