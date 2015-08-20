// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/Traits.h"

namespace honey { namespace matrix
{

/// Matrix comma initializer
template<class Matrix>
class Builder
{
public:
    Builder(Matrix& m)                              : m(m), row(0), col(0), height(0) {}
    Builder(Builder&& rhs)                          : m(rhs.m), row(rhs.row), col(rhs.col), height(rhs.height) { rhs.height = 0; }

    ~Builder()
    {
        if (height == 0) return; //Do nothing if no elements are set
        assert(row + height == m.rows() && col == m.cols(), sout()
                    << "Assigned too few matrix elements in comma initializer.\n"
                    << "Matrix size: (" << m.rows() << ", " << m.cols() << ")\n"
                    << "Cursor index: (" << row << ", " << col << ")\n"
                    << "Current row height: " << height);
    }

    /// Append builder
    template<class T>
    Builder& operator,(const Builder<T>& rhs)       { return operator,(rhs.eval()); }

    /// Append scalar
    Builder& operator,(typename Matrix::Real rhs)
    {
        preAppend(1, 1);
        m(row, col++) = rhs;
        return *this;
    }

    /// Append matrix
    template<class T>
    Builder& operator,(const MatrixBase<T>& rhs)
    {
        typedef MatrixBase<T> Rhs;
        preAppend(rhs.rows(), rhs.cols());
        if (Rhs::s_size != matrix::dynamic)
            m.block<Rhs::s_rows, Rhs::s_cols>(row, col) = rhs;
        else
            m.block(row, col, rhs.rows(), rhs.cols()) = rhs;
        col += rhs.cols();
        return *this;
    }

    /// Get initialized matrix
    Matrix& eval() const                            { return m; }
    /// Get initialized matrix
    operator Matrix&() const                        { return eval(); }

private:
    void preAppend(sdt rows, sdt cols)
    {
        mt_unused(cols);
        if (col == m.cols())
        {
            //Begin new row
            row += height;
            height = 0;
            col = 0;
        }
        assert(row + rows <= m.rows() && col + cols <= m.cols(), sout()
                    << "Block assignment in comma initializer out of matrix bounds.\n"
                    << "Matrix size: (" << m.rows() << ", " << m.cols() << ")\n"
                    << "Cursor index: (" << row << ", " << col << ")\n"
                    << "Block size: (" << rows << ", " << cols << ")");
        //Ensure any previously set blocks can't be overwritten
        if (height < rows) height = rows;
    }

    Matrix& m;
    sdt row;
    sdt col;
    sdt height;
};

} }

