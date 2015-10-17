// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/NumAnalysis/Qrd.h"

namespace honey
{

template<class Real>
void Qrd<Real>::householder(Matrix& a, Matrix& q, Matrix& r)
{
    // Calc Householder vectors (lower trapazoidal) and R (upper triangular) of A in place
    sdt m = a.rows(), n = a.cols();
    for (sdt k = 0; k < n; ++k)
    {
        // Compute 2-norm of k-th column without under/overflow.
        Double nrm = 0;
        for (sdt i = k; i < m; ++i)
            nrm = Alge_d::hypot(nrm,a(i,k));

        if (nrm != 0)
        {
            // Form k-th Householder vector.
            if (a(k,k) < 0)
                nrm = -nrm;
            for (sdt i = k; i < m; ++i)
                a(i,k) /= nrm;
            a(k,k) += 1;

            // Apply transformation to remaining columns.
            for (sdt j = k+1; j < n; ++j)
            {
                Double s = 0;
                for (sdt i = k; i < m; ++i)
                    s += a(i,k)*a(i,j);
                s = -s/a(k,k);
                for (sdt i = k; i < m; ++i)
                    a(i,j) += s*a(i,k);
            }
        }
        //Set R diagonal
        r(k,k) = -nrm;
    }

    //Copy to R and zero out Householder vectors above the diagonal
    for (sdt i = 0; i < n; ++i)
        for (sdt j = i+1; j < n; ++j)
        {
            r(i,j) = a(i,j);
            a(i,j) = 0;
        }
    //Zero out R below the diagonal
    for (sdt i = 0; i < n; ++i)
        for (sdt j = 0; j < i; ++j)
            r(i,j) = 0;

    //Calc Q
    sdt n1 = q.cols();
    q.fromIdentity();
    for (sdt k = n-1; k >= 0; --k)
    {
        for (sdt j = k; j < n1; ++j)
        {
            if (a(k,k) == 0) continue;
            Double s = 0;
            for (sdt i = k; i < m; ++i)
                s += a(i,k)*q(i,j);
            s = -s/a(k,k);
            for (sdt i = k; i < m; ++i)
                q(i,j) += s*a(i,k);
        }
    }
}

template class Qrd<Float>;
template class Qrd<Double>;
template class Qrd<Quad>;

}
