// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Optional.h"
#include "Honey/Math/Alge/Matrix/Builder.h"
#include "Honey/Math/Alge/Matrix/Iter.h"
#include "Honey/Math/Alge/Matrix/Block.h"

namespace honey
{

template<class Real> class Alge_;
template<class Real> class Trig_;
template<class Real> class Svd;

/// Matrix base class
template<class Subclass>
class MatrixBase : public matrix::priv::Traits<Subclass>::Storage
{
protected:
    typedef typename matrix::priv::Traits<Subclass>::Storage Storage;
public:
    typedef Subclass                        MatrixS;
    using typename Storage::Real;
protected:
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    typedef Alge_<Double>                   Alge_d;
    typedef Trig_<Real>                     Trig;
    typedef Svd<Real>                       Svd;
    using Storage::assertSize;
    using Storage::assertIndex;
    
public:
    using Storage::s_rows;
    using Storage::s_cols;
    using Storage::s_size;
    using Storage::options;
    using typename Storage::Alloc;
    using Storage::subc;
    using Storage::operator[];
    using Storage::operator();
    using Storage::rows;
    using Storage::cols;
    using Storage::size;
    
    typedef Vec<s_rows, Real>                           VecCol;
    typedef Vec<s_cols, Real, matrix::Option::vecRow>   VecRow;

    /// Initialize from array. If the array is in row-major format set rowMajor to true, otherwise set to false for column-major.
    template<class Num>
    MatrixS& fromArray(const Num* a, bool rowMajor = true)
    {
        assert(a, "Array null");
        if (!rowMajor) return subc().fromColMajor(a);
        for (sdt i = 0, end = size(); i < end; ++i) m(i) = (Real)a[i];
        return subc();
    }

    MatrixS& fromArray(const Real* a, bool rowMajor = true)
    {
        assert(a, "Array null");
        if (!rowMajor) return subc().fromColMajor(a);
        matrix::priv::storageCopy(a, subc());
        return subc();
    }

    /// Zero all elements
    MatrixS& fromZero()                                                 { matrix::priv::storageFillZero(subc()); return subc(); }
    /// Initialize with scalar in every element
    MatrixS& fromScalar(Real f)                                         { matrix::priv::storageFill(subc(), f); return subc(); }

    /// Make matrix identity.  For non-square matrices the identity is in the upper-left square block, the rest is filled with zero.
    MatrixS& fromIdentity()
    {
        fromZero();
        sdt n = Alge::min(rows(), cols());
        for (sdt i = 0; i < n; ++i) m(i,i) = 1;
        return subc();
    }

    /// Initialize matrix elements from scalars and other matrices using the comma operator: Matrix() << 1, 2, vec*5 ... The number of assigned elements must total up to be the matrix size exactly.
    /**
      * When used with matrices the initializer works by setting blocks.
      * As the cursor moves along the row, each block is copied in until all columns are filled, then the next row begins.
      * The largest block height determines where the next row begins, so it is impossible to overwrite previous blocks.
      *
      *     Matrix<4,4>() << mat2x2, mat2x2, mat2x2, vec2, vec2;    //Completely filled
      *     Matrix<4,4>() << 5, 10, mat2x2, mat1x2, mat2x2;         //The four elements at (1,0),(1,1),(3,0),(3,1) are not filled.
      */
    template<class T>
    matrix::Builder<MatrixS> operator<<(T&& val)                        { return move(matrix::Builder<MatrixS>(subc()).operator,(forward<T>(val))); }

    /// Assign to matrix of any size. Asserts that any fixed dimensions in this matrix match those in rhs.
    template<class T>
    MatrixBase& operator=(const MatrixBase<T>& rhs)
    {
        typedef MatrixBase<T> Rhs;
        static_assert(  (s_rows == matrix::dynamic || Rhs::s_rows == matrix::dynamic || s_rows == Rhs::s_rows) &&
                        (s_cols == matrix::dynamic || Rhs::s_cols == matrix::dynamic || s_cols == Rhs::s_cols),
                        "Can only assign to matrix of the same size");

        resize(rhs.rows(), rhs.cols());
        matrix::priv::storageCopy(rhs.subc(), subc());
        return *this;
    }

    /// Convert to a matrix of another real type
    template<class Matrix>
    Matrix cast()
    {
        auto ret = Matrix().resize(rows(), cols());
        matrix::priv::storageTransform(subc(), ret, [](Real e) { return typename Matrix::Real(e); });
        return ret;
    }

    template<class T>
    bool operator==(const MatrixBase<T>& rhs) const
    {
        typedef MatrixBase<T> Rhs;
        static_assert(s_size == matrix::dynamic || Rhs::s_size == matrix::dynamic || s_size == Rhs::s_size, "can't compare different sized matrices");
        assert(size() == rhs.size(), "Can't compare different sized matrices");

        return matrix::priv::storageEqual(subc(), rhs.subc());
    }

    template<class T>
    bool operator!=(const MatrixBase<T>& rhs) const                     { return !operator==(rhs.subc()); }
    template<class T>
    bool operator< (const MatrixBase<T>& rhs) const                     { return find(subc(), rhs.subc(), [](Real e, Real rhs) { return e >= rhs; }) == end(); }
    template<class T>
    bool operator> (const MatrixBase<T>& rhs) const                     { return find(subc(), rhs.subc(), [](Real e, Real rhs) { return e <= rhs; }) == end(); }
    template<class T>
    bool operator<=(const MatrixBase<T>& rhs) const                     { return find(subc(), rhs.subc(), [](Real e, Real rhs) { return e > rhs; }) == end(); }
    template<class T>
    bool operator>=(const MatrixBase<T>& rhs) const                     { return find(subc(), rhs.subc(), [](Real e, Real rhs) { return e < rhs; }) == end(); }
    
    /// Add another matrix.  Stores result in and returns res.  res may reference the same matrix as this or rhs.
    template<class T, class Res>
    Res&& add(const T& rhs, Res&& res) const                            { return map(subc(), rhs.subc(), forward<Res>(res.resize(rows(),cols())), [](Real e, Real rhs) { return e + rhs; }); }

    MatrixS  operator+ () const                                         { return subc(); }
    template<class T>
    MatrixS  operator+ (const MatrixBase<T>& rhs) const                 { return add(rhs.subc(), MatrixS()); }
    template<class T>
    MatrixS& operator+=(const MatrixBase<T>& rhs)                       { return add(rhs.subc(), subc()); }

    /// Subtract another matrix.  Stores result in and returns res.  res may reference the same matrix as this or rhs.
    template<class T, class Res>
    Res&& sub(const T& rhs, Res&& res) const                            { return map(subc(), rhs.subc(), forward<Res>(res.resize(rows(),cols())), [](Real e, Real rhs) { return e - rhs; }); }

    MatrixS  operator- () const                                         { return map(subc(), MatrixS().resize(rows(),cols()), [](Real e) { return -e; }); }
    template<class T>
    MatrixS  operator- (const MatrixBase<T>& rhs) const                 { return sub(rhs.subc(), MatrixS()); }
    template<class T>
    MatrixS& operator-=(const MatrixBase<T>& rhs)                       { return sub(rhs.subc(), subc()); }

    /// Multiply with another matrix.  This mat's column size must match rhs' row size.  Stores result in and returns res.  Complexity: \f$ O(m n\ rhs_n) \f$
    template<class T, class Res>
    Res&& mul(const T& rhs, Res&& res) const
    {
        static_assert(  s_cols == matrix::dynamic || T::s_rows == matrix::dynamic ||
                        s_cols == T::s_rows, "Concatenation invalid with rhs dimensions");
        assert(cols() == rhs.rows(), "Concatenation invalid with rhs dimensions");

        res.resize(rows(), rhs.cols()).fromZero();
        const sdt rows = this->rows(), cols = this->cols(), cols2 = rhs.cols(); 
        for (sdt i = 0; i < rows; ++i)
            for (sdt j = 0; j < cols2; ++j)
                for (sdt k = 0; k < cols; ++k)
                    res(i,j) += m(i,k) * rhs(k,j);
        return forward<Res>(res);
    }

    /// Multiply with another matrix. Returns a new matrix.
    template<class T>
    Matrix<s_rows, T::s_cols, Real> operator*(const T& rhs) const       { return subc().mul(rhs, Matrix<s_rows, T::s_cols, Real>()); }
    MatrixS  operator* (Real rhs) const                                 { return map(subc(), MatrixS().resize(rows(),cols()), [&](Real e) { return e * rhs; }); }
    template<class T>
    MatrixS& operator*=(const MatrixBase<T>& rhs)                       { return subc().operator=(subc().operator*(rhs.subc())); }
    MatrixS& operator*=(Real rhs)                                       { return map(subc(), subc(), [&](Real e) { return e * rhs; }); }

    MatrixS  operator/ (Real rhs) const                                 { return operator*(1 / rhs); }
    MatrixS& operator/=(Real rhs)                                       { return operator*=(1 / rhs); }

    friend MatrixS operator*(Real lhs, const MatrixBase& rhs)           { return rhs.subc().operator*(lhs); }

    /// Add rhs to each element. Returns a new matrix with the results.
    MatrixS  elemAdd(Real rhs) const                                    { return map(subc(), MatrixS().resize(rows(),cols()), [&](Real e) { return e + rhs; }); }
    /// Add rhs to each element
    MatrixS& elemAddEq(Real rhs)                                        { return map(subc(), subc(), [&](Real e) { return e + rhs; }); }
    /// Subtract rhs from each element. Returns a new matrix with the results.
    MatrixS  elemSub(Real rhs) const                                    { return map(subc(), MatrixS().resize(rows(),cols()), [&](Real e) { return e - rhs; }); }
    /// Subtract rhs from each element
    MatrixS& elemSubEq(Real rhs)                                        { return map(subc(), subc(), [&](Real e) { return e - rhs; }); }
    /// Multiply each element with its corresponding element in rhs. Returns a new matrix with the results.
    template<class T>
    MatrixS elemMul(const MatrixBase<T>& rhs) const                     { return map(subc(), rhs.subc(), MatrixS().resize(rows(),cols()), [](Real e, Real rhs) { return e * rhs; }); }
    /// Multiply each element with its corresponding element in rhs
    template<class T>
    MatrixS& elemMulEq(const MatrixBase<T>& rhs)                        { return map(subc(), rhs.subc(), subc(), [](Real e, Real rhs) { return e * rhs; }); }
    /// Divide each element by its corresponding element in rhs. Returns a new matrix with the results.
    template<class T>
    MatrixS elemDiv(const MatrixBase<T>& rhs) const                     { return map(subc(), rhs.subc(), MatrixS().resize(rows(),cols()), [](Real e, Real rhs) { return e / rhs; }); }
    /// Divide each element by its corresponding element in rhs
    template<class T>
    MatrixS& elemDivEq(const MatrixBase<T>& rhs)                        { return map(subc(), rhs.subc(), subc(), [](Real e, Real rhs) { return e / rhs; }); }
    /// Get the absolute value of each element. Returns a new matrix with the results.
    MatrixS elemAbs() const                                             { return map(subc(), MatrixS().resize(rows(),cols()), [](Real e) { return Alge::abs(e); }); }
    /// Square each element. Returns a new matrix with the results.
    MatrixS elemSqr() const                                             { return map(subc(), MatrixS().resize(rows(),cols()), [](Real e) { return e * e; }); }
    /// Inverse each element. Returns a new matrix with the results.
    MatrixS elemInverse() const                                         { return map(subc(), MatrixS().resize(rows(),cols()), [](Real e) { return 1 / e; }); }
    /// Get the min of each element and its corresponding element in rhs. Returns a new matrix with the results.
    template<class T>
    MatrixS elemMin(const MatrixBase<T>& rhs) const                     { return map(subc(), rhs.subc(), MatrixS().resize(rows(),cols()), [](Real e, Real rhs) { return Alge::min(e,rhs); }); }
    /// Get the max of each element and its corresponding element in rhs. Returns a new matrix with the results.
    template<class T>
    MatrixS elemMax(const MatrixBase<T>& rhs) const                     { return map(subc(), rhs.subc(), MatrixS().resize(rows(),cols()), [](Real e, Real rhs) { return Alge::max(e,rhs); }); }
    
    /// Check if each element is exactly zero
    bool isZero() const                                                 { return find(subc(), [](Real e) { return e != 0; }) == end(); }
    /// Check if each element is close to zero
    bool isNearZero(Real tol = Real_::zeroTol) const                    { return find(subc(), [&](Real e) { return !Alge::isNearZero(e, tol); }) == end(); }
    /// Clamp each element between its corresponding elements in min and max. Returns a new matrix with the results.
    template<class T>
    MatrixS clamp(const MatrixBase<T>& min, const MatrixBase<T>& max) const
                                                                        { return map(subc(), min.subc(), max.subc(), MatrixS().resize(rows(),cols()), [](Real e, Real min, Real max) { return Alge::clamp(e, min, max); }); }

    /// Get the sum of all elements
    Real sum() const                                                    { return reduce(subc(), Real(0), [](Real a, Real e) { return a + e; }); }
    /// Get the product of all elements
    Real prod() const                                                   { return reduce(subc(), Real(1), [](Real a, Real e) { return a * e; }); }
    /// Get the mean of all elements
    Real mean() const                                                   { return sum() / size(); }
    /// Get the minimum element
    Real min() const                                                    { return reduce(subc(), Real_::inf, [](Real a, Real e) { return Alge::min(a,e); }); }
    /// Get the maximum element
    Real max() const                                                    { return reduce(subc(), -Real_::inf, [](Real a, Real e) { return Alge::max(a,e); }); }

    /// Get block at offset (row,col) with size (Rows, Cols).  If Rows or Cols is fixed then `rows` or `cols` may be left unspecified.
    /**
      * Blocks can be used like any other matrix, they access and modify sub-sections:
      *
      *     m4x4.block(...) = m2x2;                 //Assign a 2x2 sub-section of m4x4
      *     m4x4.block(...).fromZero();             //Zero out block
      *     m2x2 = m8x8.block(...).block(...);      //Blocks are fully recursive
      */
    template<sdt Rows, sdt Cols>
    matrix::Block<MatrixS,Rows,Cols>
        block(sdt row, sdt col, sdt rows = -1, sdt cols = -1)           { return matrix::Block<MatrixS,Rows,Cols>(subc(), row, col, rows, cols); }

    template<sdt Rows, sdt Cols>
    matrix::Block<const MatrixS,Rows,Cols>
        block(sdt row, sdt col, sdt rows = -1, sdt cols = -1) const     { return matrix::Block<const MatrixS,Rows,Cols>(subc(), row, col, rows, cols); }

    /// Get dynamic block at offset (row,col) with size (rows, cols)
    matrix::Block<MatrixS>
        block(sdt row, sdt col, sdt rows, sdt cols)                     { return matrix::Block<MatrixS>(subc(), row, col, rows, cols); }

    matrix::Block<const MatrixS>
        block(sdt row, sdt col, sdt rows, sdt cols) const               { return matrix::Block<const MatrixS>(subc(), row, col, rows, cols); }

    /// Get row as a row vector
    matrix::Block<MatrixS,1,s_cols>        row(sdt row)                 { return block<1,s_cols>(row, 0, 1, cols()); }
    matrix::Block<const MatrixS,1,s_cols>  row(sdt row) const           { return block<1,s_cols>(row, 0, 1, cols()); }

    /// Get column as a column vector
    matrix::Block<MatrixS,s_rows,1>        col(sdt col)                 { return block<s_rows,1>(0, col, rows(), 1); }
    matrix::Block<const MatrixS,s_rows,1>  col(sdt col) const           { return block<s_rows,1>(0, col, rows(), 1); }

    /// Sets number of rows/columns and reallocates only if the size has changed (rows*cols). All previous data is lost on reallocation. Returns self.
    /**
      * Resize with -1 in rows or cols to request no change to that dimension
      */
    MatrixS& resize(sdt rows, sdt cols)                                 { Storage::resize(rows, cols); return subc(); }

    /// Get an iterator over the elements in row-major order
    matrix::Iter<MatrixS> begin()                                       { return matrix::Iter<MatrixS>(subc(), 0); }
    matrix::Iter<const MatrixS> begin() const                           { return matrix::Iter<const MatrixS>(subc(), 0); }

    /// Get an iterator to the end of the iteration provided by begin()
    matrix::Iter<MatrixS> end()                                         { return matrix::Iter<MatrixS>(subc(), size()); }
    matrix::Iter<const MatrixS> end() const                             { return matrix::Iter<const MatrixS>(subc(), size()); }

    /// Get an iterator to the element at index
    matrix::Iter<MatrixS> iter(sdt i)                                   { assertSize(i); return begin() + i; }
    matrix::Iter<const MatrixS> iter(sdt i) const                       { assertSize(i); return begin() + i; }

    /// Get an iterator to the element at (row, col)
    matrix::Iter<MatrixS> iter(sdt row, sdt col)                        { assertSize(row,col); return begin() + (row*cols()+col); }
    matrix::Iter<const MatrixS> iter(sdt row, sdt col) const            { assertSize(row,col); return begin() + (row*cols()+col); }

    /// Copy matrix into array. If the array is in row-major format set rowMajor to true, otherwise set to false for column-major.
    template<class Num>
    Num* toArray(Num* a, bool rowMajor = true) const
    {
        assert(a, "Array null");
        if (!rowMajor) return subc().toColMajor(a);
        for (sdt i = 0, end = size(); i < end; ++i) a[i] = (Num)m(i);
        return a;
    }
    Real* toArray(Real* a, bool rowMajor = true) const
    {
        assert(a, "Array null");
        if (!rowMajor) return subc().toColMajor(a);
        matrix::priv::storageCopy(subc(), a);
        return a;
    }

    /// \f$ M^T \f$ transpose and store result in res. Returns res.
    template<class Res>
    Res&& transpose(Res&& res) const
    {
        res.resize(cols(), rows());
        const sdt rows = this->rows(), cols = this->cols();
        for (sdt i = 0; i < rows; ++i)
            for (sdt j = 0; j < cols; ++j)
                res(j,i) = m(i,j);
        return forward<Res>(res);
    }

    /// Returns new matrix
    Matrix<s_cols,s_rows,Real> transpose() const                        { return subc().transpose(Matrix<s_cols,s_rows,Real>()); }

    /// transpose and store in this matrix, only valid for square matrices
    void transposeInPlace()
    {
        static_assert(s_size == matrix::dynamic || s_rows == s_cols, "This matrix must be square");
        assert(rows() == cols(), "This matrix must be square");

        const sdt rows = this->rows();
        for (sdt i = 0; i < rows-1; ++i)
            for (sdt j = i+1; j < rows; ++j)
                std::swap(m(i*rows + j), m(i + rows*j));
    }

    /// \f$ M^T * rhs \f$ Stores result in and returns res.
    template<class T, class Res>
    Res&& transposeMul(const T& rhs, Res&& res) const
    {
        static_assert(  s_rows == matrix::dynamic || T::s_rows == matrix::dynamic ||
                        s_rows == T::s_rows, "Concatenation invalid with rhs dimensions");
        assert(rows() == rhs.rows(), "Concatenation invalid with rhs dimensions");

        res.resize(cols(), rhs.cols()).fromZero();
        const sdt rows = this->rows(), cols = this->cols(), cols2 = rhs.cols();
        for (sdt i = 0; i < cols; ++i)
            for (sdt j = 0; j < cols2; ++j)
                for (sdt k = 0; k < rows; ++k)
                    res(i,j) += m(k,i) * rhs(k,j);
        return forward<Res>(res);
    }

    /// Returns new matrix
    template<class T>
    Matrix<s_cols, T::s_cols, Real> transposeMul(const T& rhs) const    { return subc().transposeMul(rhs, Matrix<s_cols, T::s_cols, Real>()); }

    /// \f$ M * rhs^T \f$ Stores result in and returns res.
    template<class T, class Res>
    Res&& mulTranspose(const T& rhs, Res&& res) const
    {
        static_assert(  s_cols == matrix::dynamic || T::s_cols == matrix::dynamic ||
                        s_cols == T::s_cols, "Concatenation invalid with rhs dimensions");
        assert(cols() == rhs.cols(), "Concatenation invalid with rhs dimensions");

        res.resize(rows(), rhs.rows()).fromZero();
        const sdt rows = this->rows(), cols = this->cols(), rows2 = rhs.rows();
        for (sdt i = 0; i < rows; ++i)
            for (sdt j = 0; j < rows2; ++j)
                for (sdt k = 0; k < cols; ++k)
                    res(i,j) += m(i,k) * rhs(j,k);
        return forward<Res>(res);
    }

    /// Returns new matrix
    template<class T>
    Matrix<s_rows, T::s_rows, Real> mulTranspose(const T& rhs) const    { return subc().mulTranspose(rhs, Matrix<s_rows, T::s_rows, Real>()); }

    /// \f$ M^T * rhs^T = (rhs*M)^T \f$ Stores result in and returns res.
    template<class T, class Res>
    Res&& transposeMulTranspose(const T& rhs, Res&& res) const
    {
        static_assert(  s_rows == matrix::dynamic || T::s_cols == matrix::dynamic ||
                        s_rows == T::s_cols, "Concatenation invalid with rhs dimensions");
        assert(rows() == rhs.cols(), "Concatenation invalid with rhs dimensions");

        res.resize(cols(), rhs.rows()).fromZero();
        const sdt rows = this->rows(), cols = this->cols(), rows2 = rhs.rows();
        for (sdt i = 0; i < cols; ++i)
            for (sdt j = 0; j < rows2; ++j)
                for (sdt k = 0; k < rows; ++k)
                    res(i,j) += m(k,i) * rhs(j,k);
        return forward<Res>(res);
    }

    /// Returns new matrix
    template<class T>
    Matrix<s_cols, T::s_rows, Real> transposeMulTranspose(const T& rhs) const   { return subc().transposeMulTranspose(rhs, Matrix<s_cols, T::s_rows, Real>()); }

    /// Returns a matrix without the selected row and column
    
    auto minor(sdt row, sdt col) const ->
        Matrix< (s_rows > 0) ? s_rows-1 : s_rows,
                (s_cols > 0) ? s_cols-1 : s_cols, Real>
    {
        assertIndex(row, col);
        auto res = Matrix<  (s_rows > 0) ? s_rows-1 : s_rows,
                            (s_cols > 0) ? s_cols-1 : s_cols, Real>
                            ().resize(rows()-1, cols()-1);
        const sdt rows = this->rows()-row-1, cols = this->cols()-col-1;
        res.block(0,0,row,col) = block(0,0,row,col);                    //UL
        res.block(0,col,row,cols) = block(0,col+1,row,cols);            //UR
        res.block(row,0,rows,col) = block(row+1,0,rows,col);            //LL
        res.block(row,col,rows,cols) = block(row+1,col+1,rows,cols);    //LR
        return res;
    }

    /// Get the pseudo-inverse of this matrix.  The pseudo-determinant will be returned in det if specified.
    Matrix<s_cols, s_rows, Real> inverse(optional<Real&> det = optnull) const
    {
        auto res = Matrix<s_cols, s_rows, Real>().resize(cols(), rows());

        if (rows() == cols() && rows() <= 3)
        {
            //Square
            switch(rows())
            {
            case 1:
                {
                    Real d = determinant();
                    if (det) det = d;
                    if (Alge::isNearZero(d))
                    {
                        if (det) det = 0;
                        return res.fromZero();
                    }
                    res(0,0) = 1 / d;
                    break;
                }

            case 2:
                {
                    Real d = determinant();
                    if (det) det = d;
                    if (Alge::isNearZero(d))
                    {
                        if (det) det = 0;
                        return res.fromZero();
                    }
                    res(0,0) = m(1,1);
                    res(0,1) = -m(0,1);
                    res(1,0) = -m(1,0);
                    res(1,1) = m(0,0);
                    res /= d;
                    break;
                }

            case 3:
                {
                    Real d = determinant3(res);
                    if (det) det = d;
                    if (Alge::isNearZero(d))
                    {
                        if (det) det = 0;
                        return res.fromZero();
                    }
                    res /= d;
                    break;
                }
            }
        }
        else
        {
            //Non-square or > 3x3
            Svd svd(subc());
            svd.inverse(res);
            //Determinant is the product of all non-zero singular values (or 1 if all are zero)
            if (det) det = reduce(svd.w(), Real(1), [](Real a, Real e) { return !Alge::isNearZero(e) ? a*e : a; });
        }

        return res;
    }

    /// Get the determinant. Returns the pseudo-determinant if this matrix is not square.
    Real determinant() const
    {
        Real res = 0;

        if (rows() == cols())
        {
            //Square
            switch(rows())
            {
            case 1: res = m(0,0); break;
            case 2: res = m(0,0)*m(1,1) - m(1,0)*m(0,1); break;
            case 3: { MatrixS tmp(*this); res = determinant3(tmp); } break;
            default:
                {
                    for (sdt j = 0; j < cols(); j++)
                    {
                        auto minor_ = minor(0, j);
                        res += (1 - j%2 - j%2) * m(0,j) * minor_.determinant();
                    }
                    break;
                }
            }
        }
        else
        {
            //Non-square
            //Determinant is the product of all non-zero singular values (or 1 if all are zero)
            Svd svd;
            res = reduce(svd.calcValues(subc()).w(), Real(1), [](Real a, Real e) { return !Alge::isNearZero(e) ? a*e : a; });
        }
        return res;
    }

    /// Get the condition value.  A high value means the matrix is ill-conditioned and close to singular, so inversion and linear ops will be unreliable. 
    Real cond() const
    {
        //Condition is a function of the non-zero singular values: max(sv) / min(sv)
        Svd svd;
        auto bounds = reduce(svd.calcValues(subc()).w(), make_tuple(Real_::inf, -Real_::inf),
            [](tuple<Real,Real> a, Real e) { return !Alge::isNearZero(e) ? make_tuple(Alge::min(get<0>(a), e), Alge::max(get<1>(a), e)) : a; });
        return get<1>(bounds) / get<0>(bounds);
    }

    friend ostream& operator<<(ostream& os, const MatrixBase& mat)
    {
        //Get longest number for each column
        vector<int> colLen;
        auto oslen = os.tellp();
        for (auto j : range(mat.cols()))
        {
            int lenMax = 0;
            for (auto i : range(mat.rows()))
            {
                int len = (int)((os << mat(i,j)).tellp() - oslen);
                os.seekp(oslen);
                lenMax = Alge::max(lenMax, len);
            }
            colLen.push_back(lenMax);
        }
        
        os << stringstream::indentInc << "[";
        if (mat.cols() > 0)
        {
            for (auto i : range(mat.rows()))
            {
                if (i != 0) os << ",";
                os << endl;
                for (auto j : range(mat.cols()))
                {
                    if (j != 0) os << ", ";
                    os << std::setw(colLen[j]) << mat(i,j);
                }
            }
        }
        return os << stringstream::indentDec << endl << "]";
    }

protected:

    /// Access matrix element at index. Wrapper for convenience only, more readable than (*this)(i)
    const Real& m(sdt i) const                                          { return (*this)(i); }
    Real& m(sdt i)                                                      { return (*this)(i); }
    /// Access matrix element with (row, column)
    const Real& m(sdt row, sdt col) const                               { return (*this)(row, col); }
    Real& m(sdt row, sdt col)                                           { return (*this)(row, col); }

    template<class Num>
    MatrixS& fromColMajor(const Num* a)
    {
        const sdt rows = this->rows(), cols = this->cols();
        for (sdt i = 0; i < rows; ++i)
            for (sdt j = 0; j < cols; ++j)
                m(i*cols+j) = (Real)a[j*rows+i];
        return subc();
    }

    template<class Num>
    Num* toColMajor(Num* a) const
    {
        const sdt rows = this->rows(), cols = this->cols();
        for (sdt i = 0; i < rows; ++i)
            for (sdt j = 0; j < cols; ++j)
                a[j*rows+i] = (Num)m(i*cols+j);
        return a;
    }

private:

    /// 3x3 determinant
    template<class T>
    Real determinant3(MatrixBase<T>& tmp) const
    {
        tmp(0,0) = m(1,1)*m(2,2)-m(1,2)*m(2,1);
        tmp(0,1) = m(0,2)*m(2,1)-m(0,1)*m(2,2);
        tmp(0,2) = m(0,1)*m(1,2)-m(0,2)*m(1,1);
        tmp(1,0) = m(1,2)*m(2,0)-m(1,0)*m(2,2);
        tmp(1,1) = m(0,0)*m(2,2)-m(0,2)*m(2,0);
        tmp(1,2) = m(0,2)*m(1,0)-m(0,0)*m(1,2);
        tmp(2,0) = m(1,0)*m(2,1)-m(1,1)*m(2,0);
        tmp(2,1) = m(0,1)*m(2,0)-m(0,0)*m(2,1);
        tmp(2,2) = m(0,0)*m(1,1)-m(0,1)*m(1,0);
        return m(0,0)*tmp(0,0) + m(0,1)*tmp(1,0) + m(0,2)*tmp(2,0);
    }
};

}

