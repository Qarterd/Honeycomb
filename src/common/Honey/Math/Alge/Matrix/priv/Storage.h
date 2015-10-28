// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/Traits.h"

namespace honey { namespace matrix { namespace priv
{

template<class Subclass>
struct StorageDense
{
    typedef typename Traits<Subclass>::Real             Real;
    //Elem type must be tracked because array const-ness must propagate down through descendant blocks and iters
    typedef typename Traits<Subclass>::ElemT            ElemT;
    static const sdt s_rows                             = Traits<Subclass>::rows;
    static const sdt s_cols                             = Traits<Subclass>::cols;
    static const sdt s_size                             = s_rows != dynamic && s_cols != dynamic ? s_rows*s_cols : dynamic;
    static const int options                            = Traits<Subclass>::options;
    typedef typename Traits<Subclass>::Alloc            Alloc;

    static_assert((s_rows == dynamic || s_rows >= 0) && (s_cols == dynamic || s_cols >= 0), "Matrix size must be zero or greater");

    const ElemT& operator()(sdt row, sdt col) const     { return subc()(row, col); }
    ElemT& operator()(sdt row, sdt col)                 { return subc()(row, col); }

    /// Default resize, just asserts that fixed dimensions match args
    void resize(sdt rows, sdt cols)
    {
        mt_unused(rows); mt_unused(cols);
        assert(rows == -1 || s_rows == dynamic || rows == s_rows, sout() << "Can't change fixed row count from " << (sdt)s_rows << " to " << rows);
        assert(cols == -1 || s_cols == dynamic || cols == s_cols, sout() << "Can't change fixed col count from " << (sdt)s_cols << " to " << cols);
    }

    sdt rows() const                                    { return subc().rows(); }
    sdt cols() const                                    { return subc().cols(); }
    sdt size() const                                    { return subc().size(); }

    const ElemT* data() const                           { return subc().data(); }
    ElemT* data()                                       { return subc().data(); }

    /// Cast to array
    operator ElemT*()                                   { return data(); }
    operator const ElemT*() const                       { return data(); }

    /// Get the subclass that inherited from this base class
    const Subclass& subc() const                        { return static_cast<const Subclass&>(*this); }
    Subclass& subc()                                    { return static_cast<Subclass&>(*this); }

protected:
    void assertIndex(sdt i) const                       { mt_unused(i);                     assert(i >= 0 && i < size()); }
    void assertIndex(sdt row, sdt col) const            { mt_unused(row); mt_unused(col);   assert(row >= 0 && row < rows() && col >= 0 && col < cols()); }
    void assertSize(sdt size) const                     { mt_unused(size);                  assert(size >= 0 && size <= this->size()); }
    void assertSize(sdt rows, sdt cols) const           { mt_unused(rows); mt_unused(cols); assert(rows >= 0 && rows <= this->rows() && cols >= 0 && cols <= this->cols()); }
};

/// Copy between dense storages
template<class Src, class Dst>
void storageCopy(const StorageDense<Src>& src, StorageDense<Dst>& dst)
                                                        { if (!src.size()) return; std::copy_n(src.data(), src.size(), dst.data()); }
/// Copy between array and dense storage
template<class Dst>
void storageCopy(const typename StorageDense<Dst>::Real* src, StorageDense<Dst>& dst)
                                                        { if (!dst.size()) return; std::copy_n(src, dst.size(), dst.data()); }

template<class Src>
void storageCopy(const StorageDense<Src>& src, typename StorageDense<Src>::Real* dst)
                                                        { if (!src.size()) return; std::copy_n(src.data(), src.size(), dst); }

/// Transform between dense storages
template<class Src, class Dst, class Func>
void storageTransform(const StorageDense<Src>& src, StorageDense<Dst>& dst, Func&& f)
                                                        { if (!src.size()) return; std::transform(src.data(), src.data() + src.size(), dst.data(), forward<Func>(f)); }

/// Fill dense storage with scalar
template<class T>
void storageFill(StorageDense<T>& store, typename StorageDense<T>::Real f)
                                                        { if (!store.size()) return; std::fill_n(store.data(), store.size(), f); }
/// Fill dense storage with zeros
template<class T>
void storageFillZero(StorageDense<T>& store)            { if (!store.size()) return; std::fill_n(reinterpret_cast<uint8*>(store.data()), store.size()*sizeof(typename StorageDense<T>::Real), uint8(0)); }

/// Test between dense storages
template<class T, class T2>
bool storageEqual(const StorageDense<T>& lhs, const StorageDense<T2>& rhs)
{
    static_assert((std::is_same<typename StorageDense<T>::Real, typename StorageDense<T2>::Real>::value), "Different element type comparison not supported");
    if (!lhs.size()) return true;
    return memcmp(lhs.data(),rhs.data(),lhs.size()*sizeof(typename StorageDense<T>::Real)) == 0;
}

    
template<class Real, sdt Size, szt Align>
struct StorageAutoArray
{
    Real a[Size > 0 ? Size : 1];
};

template<class Real, sdt Size>
struct StorageAutoArray<Real, Size, 16>
{
    ALIGN(16) Real a[Size > 0 ? Size : 1];
};

/// Automatic (stack-compatible) dense storage
template<class Subclass>
class StorageAuto : public StorageDense<Subclass>
{
    typedef StorageDense<Subclass> Super;
public:
    using typename Super::Real;
    
    /// Access matrix element with [row][column]
    const Real* operator[](sdt row) const               { this->assertIndex(row,0); return &data()[row*cols()]; }
    Real* operator[](sdt row)                           { this->assertIndex(row,0); return &data()[row*cols()]; }
    /// Access matrix element at index
    const Real& operator()(sdt i) const                 { this->assertIndex(i); return data()[i]; }
    Real& operator()(sdt i)                             { this->assertIndex(i); return data()[i]; }
    /// Access matrix element with (row, column)
    const Real& operator()(sdt row, sdt col) const      { this->assertIndex(row,col); return data()[row*cols() + col]; }
    Real& operator()(sdt row, sdt col)                  { this->assertIndex(row,col); return data()[row*cols() + col]; }

    sdt rows() const                                    { return Super::s_rows; }
    sdt cols() const                                    { return Super::s_cols; }
    sdt size() const                                    { return Super::s_size; }

    /// Get as array
    const Real* data() const                            { return _a.a; }
    Real* data()                                        { return _a.a; }

private:
    StorageAutoArray<Real, Super::s_size, Option::getAlign<Super::options>::value> _a;
};

/// dynamic (heap) dense storage
template<class Subclass>
class StorageDynamic : public StorageDense<Subclass>
{
    typedef StorageDense<Subclass> Super;
public:
    using Super::s_rows;
    using Super::s_cols;
    using Super::s_size;
    using typename Super::Real;
    using typename Super::Alloc;
    
    /// Default is null array
    StorageDynamic()                                    { init(); }
    /// Init to copy of rhs
    StorageDynamic(const StorageDynamic& rhs)           { init(); setAllocator(rhs._alloc); operator=(rhs); }
    /// Init to rhs and set rhs array to null
    StorageDynamic(StorageDynamic&& rhs)                { init(); operator=(move(rhs)); }

    ~StorageDynamic()                                   { freeAligned(_a, _alloc); }

    StorageDynamic& operator=(const StorageDynamic& rhs)
    {
        resize(rhs.rows(), rhs.cols());
        if (size()) storageCopy(rhs, *this);
        return *this;
    }

    StorageDynamic& operator=(StorageDynamic&& rhs)
    {
        assert(s_rows == dynamic || rhs._rows == s_rows, sout() << "Can't change fixed row count from " << (sdt)s_rows << " to " << rhs._rows);
        assert(s_cols == dynamic || rhs._cols == s_cols, sout() << "Can't change fixed col count from " << (sdt)s_cols << " to " << rhs._cols);
        freeAligned(_a, _alloc);
        _a = rhs._a;
        _rows = rhs._rows;
        _cols = rhs._cols;
        _size = rhs._size;
        _alloc  = move(rhs._alloc);
        rhs.init();
        return *this;
    }

    /// Access matrix element with [row][column]
    const Real* operator[](sdt row) const               { this->assertIndex(row,0); return &data()[row*cols()]; }
    Real* operator[](sdt row)                           { this->assertIndex(row,0); return &data()[row*cols()]; }
    /// Access matrix element at index
    const Real& operator()(sdt i) const                 { this->assertIndex(i); return data()[i]; }
    Real& operator()(sdt i)                             { this->assertIndex(i); return data()[i]; }
    /// Access matrix element with (row, column)
    const Real& operator()(sdt row, sdt col) const      { this->assertIndex(row,col); return data()[row*cols() + col]; }
    Real& operator()(sdt row, sdt col)                  { this->assertIndex(row,col); return data()[row*cols() + col]; }

    void resize(sdt rows, sdt cols)
    {
        if (rows == -1) rows = _rows;
        if (cols == -1) cols = _cols;
        assert(rows >= 0 && cols >= 0, "Matrix size must be zero or greater");
        assert(s_rows == dynamic || rows == s_rows, sout() << "Can't change fixed row count from " << (sdt)s_rows << " to " << rows);
        assert(s_cols == dynamic || cols == s_cols, sout() << "Can't change fixed col count from " << (sdt)s_cols << " to " << cols);
        sdt size = rows*cols;
        _rows = rows;
        _cols = cols;
        if (size == _size) return;
        _size = size;
        freeAligned(_a, _alloc); _a = nullptr;
        if (_size == 0) return;
        _a = allocAligned<Real>(_size, Option::getAlign<Super::options>::value, _alloc);
    }

    sdt rows() const                                    { return _rows; }
    sdt cols() const                                    { return _cols; }
    sdt size() const                                    { return _size; }

    /// Get as array
    Real* data()                                        { return _a; }
    const Real* data() const                            { return _a; }

protected:
    void setAllocator(const Alloc& alloc)               { _alloc = alloc; }

private:

    void init()
    {
        _a = nullptr;
        _rows = s_rows == dynamic ? 0 : s_rows;
        _cols = s_cols == dynamic ? 0 : s_cols;
        _size = 0;                                   
    }

    Real* _a;
    sdt _rows;
    sdt _cols;
    sdt _size;
    Alloc _alloc;
};

/// Chooses storage based on traits
template<class Subclass>
struct Storage : public std::conditional<   Traits<Subclass>::rows != dynamic && Traits<Subclass>::cols != dynamic,
                                            StorageAuto<Subclass>, StorageDynamic<Subclass>>::type {};

} } }
