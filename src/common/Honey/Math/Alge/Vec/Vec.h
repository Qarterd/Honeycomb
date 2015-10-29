// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Vec/Base.h"
#include "Honey/Math/Alge/Vec/priv/Storage.h"

namespace honey
{

namespace vec { namespace priv
{
    /// N-dimensional vector traits
    template<sdt Dim, class Real_, int Options, class Alloc_>
    struct Traits
    {
        typedef priv::Storage<Vec<Dim,Real_,Options,Alloc_>> Storage;
        typedef Real_               Real;
        typedef Real                ElemT;
        static const sdt dim        = Dim;
        static const sdt rows       = Options & matrix::Option::vecRow ? 1 : dim;
        static const sdt cols       = Options & matrix::Option::vecRow ? dim : 1;
        static const int options    = Options;
        typedef Alloc_              Alloc;
    };
} }

template<sdt Dim, class Real, int Options, class Alloc>
struct matrix::priv::Traits<Vec<Dim,Real,Options,Alloc>> : vec::priv::Traits<Dim,Real,Options,Alloc> {};

/// N-dimensional vector
template<sdt Dim, class Real, int Options, class Alloc>
class Vec : public VecBase<Vec<Dim,Real,Options,Alloc>>
{
    typedef VecBase<Vec<Dim,Real,Options,Alloc>> Super;
public:
    static const sdt s_size = Super::s_size;
    
    /// No init
    Vec() {}
    /// Allocate elements for dimension size, only available if vector is dynamic.
    template<class Int>
    explicit Vec(Int dim, typename std::enable_if<std::is_integral<Int>::value && s_size == matrix::dynamic>::type*_=0)
                                                                    { mt_unused(_); this->resize(dim); }
    /// Construct uniform vector
    explicit Vec(Real scalar)                                       { this->fromScalar(scalar); }
    /// Initialize from array with dimension `dim`
    Vec(const Real* a, sdt dim)                                     { this->resize(dim); this->fromArray(a); }
    /// Construct with allocator, for a dynamic vector. Allocator element type must be int8.
    Vec(const Alloc& alloc)                                         { this->setAllocator(alloc); }
    /// Construct from row or column vector of any dimension. Asserts that if this vector has a fixed dimension then it matches rhs.
    template<class T>
    Vec(const MatrixBase<T>& rhs)                                   { operator=(rhs); }

    /// Assign to row or column vector of any dimension. Asserts that if this vector has a fixed dimension then it matches rhs.
    template<class T>
    Vec& operator=(const MatrixBase<T>& rhs)                        { Super::operator=(rhs); return *this; }
};

/// N-dimensional column vector types
typedef Vec<matrix::dynamic>           VecN;
typedef Vec<matrix::dynamic, Float>    VecN_f;
typedef Vec<matrix::dynamic, Double>   VecN_d;

/// N-dimensional row vector types
typedef Vec<matrix::dynamic, Real, matrix::Option::vecRow>      VecRowN;
typedef Vec<matrix::dynamic, Float, matrix::Option::vecRow>     VecRowN_f;
typedef Vec<matrix::dynamic, Double, matrix::Option::vecRow>    VecRowN_d;


/// It's not possible to inherit ctors, so this macro is required
#define MATRIX_VEC_ADAPTER                                                                                              \
public:                                                                                                                 \
    static const sdt s_size = Super::s_size;                                                                            \
    using typename Super::Alloc;                                                                                        \
                                                                                                                        \
    Matrix() {}                                                                                                         \
    template<class Int>                                                                                                 \
    explicit Matrix(Int dim, typename std::enable_if<std::is_integral<Int>::value && s_size == matrix::dynamic>::type*_=0)  \
                                                                    : Super(dim) {}                                     \
    explicit Matrix(Real scalar)                                    : Super(scalar) {}                                  \
    Matrix(const Real* a, sdt dim)                                  : Super(a,dim) {}                                   \
    Matrix(const Alloc& alloc)                                      : Super(alloc) {}                                   \
    template<class T>                                                                                                   \
    Matrix(const MatrixBase<T>& rhs)                                : Super(rhs) {}                                     \
    template<class T>                                                                                                   \
    Matrix& operator=(const MatrixBase<T>& rhs)                     { Super::operator=(rhs); return *this; }            \

/// Matrix column vector 
template<sdt Dim, class Real, int Options, class Alloc_>
class Matrix<Dim,1,Real,Options,Alloc_> : public Vec<Dim,Real,Options,Alloc_>
{
    typedef Vec<Dim,Real,Options,Alloc_> Super;
    MATRIX_VEC_ADAPTER
};

/// Matrix row vector
template<sdt Dim, class Real, int Options, class Alloc_>
class Matrix<1,Dim,Real,Options,Alloc_> : public Vec<Dim,Real, Options | matrix::Option::vecRow, Alloc_>
{
    typedef Vec<Dim,Real, Options | matrix::Option::vecRow, Alloc_> Super;
    MATRIX_VEC_ADAPTER
};

}
