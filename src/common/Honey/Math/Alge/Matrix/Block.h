// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/priv/Storage.h"

namespace honey
{

namespace matrix
{

namespace priv
{
    /// Block wrapper around any constant matrix with auto / dynamic dense storage type.  Fully recursive, a block can wrap a block.
    template<class Subclass>
    class StorageBlock : public StorageDense<Subclass>
    {
        typedef StorageDense<Subclass> Super;
    public:
        using Super::s_rows;
        using Super::s_cols;
        using typename Super::ElemT;
        typedef typename Traits<Subclass>::MatrixP          MatrixP;

        ElemT* operator[](sdt row) const                    { this->assertIndex(row,0);   return &parent()(_row + row, _col); }
        ElemT& operator()(sdt i) const                      { this->assertIndex(i);       return parent()(_row + i / _cols, _col + i % _cols); }
        ElemT& operator()(sdt row, sdt col) const           { this->assertIndex(row,col); return parent()(_row + row, _col + col); }

        /// Get as array.  Top-left corner of block sub-section is at index 0 
        ElemT* data() const                                 { return &parent()(row(),col()); }

        /// Get parent matrix that contains this sub-block
        MatrixP& parent() const                             { return *_m; }
        /// Get row offset
        sdt row() const                                     { return _row; }
        /// Get column offset
        sdt col() const                                     { return _col; }
        /// Get row size
        sdt rows() const                                    { return _rows; }
        /// Get column size
        sdt cols() const                                    { return _cols; }
        /// Get size
        sdt size() const                                    { return _size; }

    protected:
        void initBlock(MatrixP& m, sdt row, sdt col, sdt rows, sdt cols)
        {
            _m = &m;
            _row = row;
            _col = col;
            _rows = s_rows == dynamic ? rows : s_rows;
            _cols = s_cols == dynamic ? cols : s_cols;
            _size = _rows*_cols;

            assert(_rows >= 0 && _cols >= 0, "Block size must be zero or greater");
            assert(_row >= 0 && _row + _rows <= _m->rows(), sout()
                        << "Block row bounds out of matrix range. Matrix rows: " << _m->rows()
                        << " ; Block range: [" << _row << ", " << _row + _rows << ")");
            assert(_col >= 0 && _col + _cols <= _m->cols(), sout()
                        << "Block column bounds out of matrix range. Matrix columns: " << _m->cols()
                        << " ; Block range: [" << _col << ", " << _col + _cols << ")");
        }

    private:
        MatrixP* _m;
        sdt _row;
        sdt _col;
        sdt _rows;
        sdt _cols;
        sdt _size;
    };

    /// Copy by row between dense storages
    template<class Src, class Dst>
    void storageRowCopy(const StorageDense<Src>& src, StorageDense<Dst>& dst)
    {
        if (!src.size()) return;
        for (sdt i = 0; i < src.rows(); ++i)
            std::copy_n(&src(i, 0), src.cols(), &dst(i, 0));
    }

    /// Copy between block and dense storages
    template<class Src, class Dst>
    void storageCopy(const StorageBlock<Src>& src, StorageBlock<Dst>& dst)      { storageRowCopy(src, dst); }
    template<class Src, class Dst>
    void storageCopy(const StorageBlock<Src>& src, StorageDense<Dst>& dst)      { storageRowCopy(src, dst); }
    template<class Src, class Dst>
    void storageCopy(const StorageDense<Src>& src, StorageBlock<Dst>& dst)      { storageRowCopy(src, dst); }

    /// Copy between array and block storage 
    template<class Dst>
    void storageCopy(const typename StorageBlock<Dst>::Real* src, StorageBlock<Dst>& dst) 
    {
        if (!dst.size()) return;
        for (sdt i = 0; i < dst.rows(); ++i)
            std::copy_n(src + i*dst.cols(), dst.cols(), &dst(i, 0));
    }
    template<class Src>
    void storageCopy(const StorageBlock<Src>& src, typename StorageBlock<Src>::Real* dst)
    {
        if (!src.size()) return;
        for (sdt i = 0; i < src.rows(); ++i)
            std::copy_n(&src(i, 0), src.cols(), dst + i*src.cols());
    }

    /// Tranform by row between dense storages
    template<class Src, class Dst, class Func>
    void storageRowTransform(const StorageDense<Src>& src, StorageDense<Dst>& dst, const Func& f)
    {
        if (!src.size()) return;
        for (sdt i = 0; i < src.rows(); ++i)
        {
            auto srcRow = &src(i, 0);
            std::transform(srcRow, srcRow + src.cols(), &dst(i, 0), f);
        }
    }

    /// Transform between block and dense storages
    template<class Src, class Dst, class Func>
    void storageTransform(const StorageBlock<Src>& src, StorageBlock<Dst>& dst, Func&& f)   { storageRowTransform(src, dst, f); }
    template<class Src, class Dst, class Func>
    void storageTransform(const StorageBlock<Src>& src, StorageDense<Dst>& dst, Func&& f)   { storageRowTransform(src, dst, f); }
    template<class Src, class Dst, class Func>
    void storageTransform(const StorageDense<Src>& src, StorageBlock<Dst>& dst, Func&& f)   { storageRowTransform(src, dst, f); }

    /// Fill block storage with scalar
    template<class T>
    void storageFill(StorageBlock<T>& store, typename StorageBlock<T>::Real f)
    {
        if (!store.size()) return;
        for (sdt i = 0; i < store.rows(); ++i)
            std::fill_n(&store(i, 0), store.cols(), f);
    }
    /// Fill block storage with zeros
    template<class T>
    void storageFillZero(StorageBlock<T>& store)
    {
        if (!store.size()) return;
        for (sdt i = 0; i < store.rows(); ++i)
            std::fill_n(reinterpret_cast<uint8*>(&store(i, 0)), store.cols()*sizeof(typename StorageBlock<T>::Real), uint8(0));
    }

    /// Test by row between dense storages
    template<class T, class T2>
    bool storageRowEqual(const StorageDense<T>& lhs, const StorageDense<T2>& rhs)
    {
        static_assert((std::is_same<typename StorageDense<T>::Real, typename StorageDense<T2>::Real>::value), "Comparing different element types not supported");
        if (!lhs.size()) return true;
        for (sdt i = 0; i < lhs.rows(); ++i)
            if (memcmp(&lhs(i,0), &rhs(i,0), lhs.cols()*sizeof(typename StorageDense<T>::Real)) != 0)
                return false;
        return true;
    }

    /// Test between block storages
    template<class T, class T2>
    bool storageEqual(const StorageBlock<T>& lhs, const StorageBlock<T2>& rhs)      { return storageRowEqual(lhs, rhs); }
    /// Test between block and dense storages
    template<class T, class T2>
    bool storageEqual(const StorageBlock<T>& lhs, const StorageDense<T2>& rhs)      { return storageRowEqual(lhs, rhs); }
    template<class T, class T2>
    bool storageEqual(const StorageDense<T>& lhs, const StorageBlock<T2>& rhs)      { return storageRowEqual(lhs, rhs); }


    template<class MatrixP_, sdt Rows, sdt Cols>
    struct BlockTraits
    {
        typedef Block<MatrixP_,Rows,Cols>       Subclass;
        typedef MatrixBase<Subclass>            Base;
        typedef StorageBlock<Subclass>          Storage;
        typedef MatrixP_                        MatrixP;
        typedef typename MatrixP::Real          Real;
        /// Our element access is const if matrix is const
        typedef typename std::conditional<std::is_const<MatrixP>::value, const typename MatrixP::ElemT, typename MatrixP::ElemT>::type
                                                ElemT;
        static const sdt rows                   = Rows;
        static const sdt cols                   = Cols;
        static const int options                = MatrixP::options;
        typedef typename MatrixP::Alloc         Alloc;
    };

    template<class MatrixP, sdt Rows, sdt Cols>
    struct Traits<Block<MatrixP,Rows,Cols>> : BlockTraits<MatrixP,Rows,Cols> {};
}

/// Matrix block view
template<class MatrixP, sdt s_rows, sdt s_cols>
class Block : public priv::Traits<Block<MatrixP,s_rows,s_cols>>::Base
{
    typedef typename priv::Traits<Block<MatrixP,s_rows,s_cols>>::Base Super;
public:
    typedef Matrix<s_rows,s_cols,typename Super::Real,Super::options,typename Super::Alloc> MatrixEval;

    Block() {}
    Block(MatrixP& m, sdt row, sdt col, sdt rows = -1, sdt cols = -1)
                                                            { this->initBlock(m, row, col, rows, cols); }

    /// Assign to matrix
    template<class T>
    Block& operator=(const MatrixBase<T>& rhs)              { Super::operator=(rhs.subc()); return *this; }

    /// Convert to matrix
    MatrixEval eval() const                                 { return MatrixEval(*this); }
    /// Convert to matrix
    operator MatrixEval() const                             { return eval(); }
};

} //namespace matrix

namespace vec { namespace priv
{
    /// Vector segment view
    template<class Vec, sdt Dim>
    struct Segment
    {
        typedef matrix::priv::Traits<typename std::remove_const<Vec>::type> Traits;
        typedef matrix::Block<Vec,  Traits::cols == 1 ? Dim : 1,
                                    Traits::cols == 1 ? 1 : Dim> type;

        static type create(Vec& v, sdt i, sdt dim = -1)     { return Traits::cols == 1 ? type(v,i,0,dim,1) : type(v,0,i,1,dim); }
    };
} }

}
