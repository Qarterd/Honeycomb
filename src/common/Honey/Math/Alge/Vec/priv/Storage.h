// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Matrix/priv/Storage.h"

namespace honey { namespace vec { namespace priv
{

template<class Real, sdt Dim, szt Align> struct StorageFieldsMixin;

/// Automatic (stack-compatible) vector storage that allows direct access to dimension fields
template<class Subclass>
class StorageFields :   public matrix::priv::StorageDense<Subclass>,
                        public StorageFieldsMixin<  typename matrix::priv::Traits<Subclass>::Real,
                                                    matrix::priv::Traits<Subclass>::dim,
                                                    matrix::Option::getAlign< matrix::priv::Traits<Subclass>::options >::value >
{
    typedef matrix::priv::StorageDense<Subclass> Super;
public:
    using Super::s_rows;
    using Super::s_cols;
    using Super::s_size;
    using typename Super::Real;
    
    /// Access vector element at index
    const Real& operator[](sdt i) const                 { this->assertIndex(i); return data()[i]; }
    Real& operator[](sdt i)                             { this->assertIndex(i); return data()[i]; }
    /// Access vector element at index
    const Real& operator()(sdt i) const                 { return (*this)[i]; }
    Real& operator()(sdt i)                             { return (*this)[i]; }
    /// Access vector element with (row, column)
    const Real& operator()(sdt row, sdt col) const      { this->assertIndex(row,col); return data()[row|col]; }
    Real& operator()(sdt row, sdt col)                  { this->assertIndex(row,col); return data()[row|col]; }

    sdt rows() const                                    { return s_rows; }
    sdt cols() const                                    { return s_cols; }
    sdt size() const                                    { return s_size; }

    Real* data()                                        { return &this->x; }
    const Real* data() const                            { return &this->x; }
};

/// Auto or dynamic vector storage
template<class Subclass>
struct Storage : matrix::priv::Storage<Subclass>
{
    typedef matrix::priv::Storage<Subclass> Super;
    using Super::data;
    using typename Super::Real;
    
    /// Access vector element at index
    const Real& operator[](sdt i) const                 { this->assertIndex(i); return data()[i]; }
    Real& operator[](sdt i)                             { this->assertIndex(i); return data()[i]; }

    using Super::operator();
    /// Access vector element with (row, column)
    const Real& operator()(sdt row, sdt col) const      { this->assertIndex(row,col); return data()[row|col]; }
    Real& operator()(sdt row, sdt col)                  { this->assertIndex(row,col); return data()[row|col]; }
};

} } }
