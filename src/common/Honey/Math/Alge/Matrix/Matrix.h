// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/Base.h"

namespace honey
{

/// (m x n)-dimensional matrix traits
template<sdt Rows, sdt Cols, class Real_, int Options, class Alloc_>
struct matrix::priv::Traits<Matrix<Rows,Cols,Real_,Options,Alloc_>>
{
    typedef Storage<Matrix<Rows,Cols,Real_,Options,Alloc_>> Storage;
    typedef Real_               Real;
    typedef Real                ElemT;
    static const sdt rows      = Rows;
    static const sdt cols      = Cols;
    static const int options    = Options;
    typedef Alloc_              Alloc;
};

/// (m x n)-dimensional matrix
template<sdt Rows, sdt Cols, class Real, int Options, class Alloc>
class Matrix : public MatrixBase<Matrix<Rows,Cols,Real,Options,Alloc>>
{
    typedef MatrixBase<Matrix<Rows,Cols,Real,Options,Alloc>> Super;
public:
    /// No init
    Matrix()                                                            {}
    /// Allocate elements for dimension sizes. Asserts that any fixed dimensions match rows / cols.
    Matrix(sdt rows, sdt cols)                                        { this->resize(rows, cols); }
    /// Initialize with scalar in every element
    explicit Matrix(Real scalar)                                        { this->fromScalar(scalar); }
    /// Initialize from array with dimensions (rows x cols). If the array is in row-major format set rowMajor to true, otherwise set to false for column-major.
    Matrix(const Real* a, sdt rows, sdt cols, bool rowMajor = true)   { this->resize(rows, cols); this->fromArray(a, rowMajor); }
    /// Construct with allocator, for a dynamic matrix. Allocator element type must be int8.
    Matrix(const Alloc& alloc)                                          { this->setAllocator(alloc); }
    /// Construct from matrix of any size. Asserts that any fixed dimensions in this matrix match those in rhs.
    template<class T>
    Matrix(const MatrixBase<T>& rhs)                                    { operator=(rhs); }

    /// Assign to matrix of any size. Asserts that any fixed dimensions in this matrix match those in rhs.
    template<class T>
    Matrix& operator=(const MatrixBase<T>& rhs)                         { Super::operator=(rhs.subc()); return *this; }
};

/// (m x n)-dimensional matrix types
typedef Matrix<matrix::dynamic, matrix::dynamic>            MatrixN;
typedef Matrix<matrix::dynamic, matrix::dynamic, Float>     MatrixN_f;
typedef Matrix<matrix::dynamic, matrix::dynamic, Double>    MatrixN_d;

}
