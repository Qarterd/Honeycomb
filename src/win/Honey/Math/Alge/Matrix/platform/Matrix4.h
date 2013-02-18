// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#ifdef HONEY_DX9
#ifdef Section_Header

namespace honey
{

template<> Matrix<4,4,Float>&           Matrix<4,4,Float>::mul(const Matrix& rhs, Matrix& res) const;
template<> Matrix<4,4,Float>&           Matrix<4,4,Float>::transposeMul(const Matrix& rhs, Matrix& res) const;
template<> Matrix<4,4,Float>&           Matrix<4,4,Float>::mulTranspose(const Matrix& rhs, Matrix& res) const;
template<> Matrix<4,4,Float>&           Matrix<4,4,Float>::transposeMulTranspose(const Matrix& rhs, Matrix& res) const;
template<> Matrix<4,4,Float>            Matrix<4,4,Float>::inverse(option<Real&> det) const;
template<> Matrix<4,4,Float>::Real      Matrix<4,4,Float>::determinant() const;
template<> void                         Matrix<4,4,Float>::decompose(   option<Vec3&> trans, option<Quat&> rot,
                                                                        option<Vec3&> scale, option<Quat&> skew) const;

}

#endif

#ifdef Section_Source

namespace honey
{

static FLOAT*               DX(Float& f)                { return reinterpret_cast<FLOAT*>(&f); }
static D3DXVECTOR3*         DX(Vec3_f& v)               { return reinterpret_cast<D3DXVECTOR3*>(&v); }
static D3DXQUATERNION*      DX(Quat_f& q)               { return reinterpret_cast<D3DXQUATERNION*>(&q); }
static D3DXMATRIX*          DX(Matrix4_f& m)            { return reinterpret_cast<D3DXMATRIX*>(&m); }
static const D3DXMATRIX*    DX(const Matrix4_f& m)      { return reinterpret_cast<const D3DXMATRIX*>(&m); }

template<> Matrix<4,4,Float>& Matrix<4,4,Float>::mul(const Matrix& rhs, Matrix& res) const
{
    D3DXMatrixMultiply(DX(res), DX(*this), DX(rhs));
    return res;
}

template<> Matrix<4,4,Float>& Matrix<4,4,Float>::transposeMul(const Matrix& rhs, Matrix& res) const
{
    Matrix trans = transpose();
    D3DXMatrixMultiply(DX(res), DX(trans), DX(rhs));
    return res;
}

template<> Matrix<4,4,Float>& Matrix<4,4,Float>::mulTranspose(const Matrix& rhs, Matrix& res) const
{
    Matrix trans = rhs.transpose();
    D3DXMatrixMultiply(DX(res), DX(*this), DX(trans));
    return res;
}

template<> Matrix<4,4,Float>& Matrix<4,4,Float>::transposeMulTranspose(const Matrix& rhs, Matrix& res) const
{
    D3DXMatrixMultiplyTranspose(DX(res), DX(rhs), DX(*this));
    return res;
}

template<> Matrix<4,4,Float> Matrix<4,4,Float>::inverse(option<Real&> det) const
{
    Matrix ret;
    D3DXMatrixInverse(DX(ret), det ? DX(*det) : nullptr, DX(*this));
    return ret;
}

template<> Matrix<4,4,Float>::Real Matrix<4,4,Float>::determinant() const
{
    return D3DXMatrixDeterminant(DX(*this));
}

template<> void Matrix<4,4,Float>::decompose(   option<Vec3&> trans_, option<Quat&> rot_,
                                                option<Vec3&> scale_, option<Quat&> skew) const
{
    Vec3 trans, scale; Quat rot;
    D3DXMatrixDecompose(DX(scale), DX(rot), DX(trans), DX(*this));

    if (skew)
    {
        //Skew algorithm is expensive, only do it if scale is non-uniform.
        Real tol = scale.x * 1.0e-4;
        if (!Alge::isNear(scale.x, scale.y, tol) || !Alge::isNear(scale.x, scale.z, tol))
        {
            decomposeSkew(trans_, rot_, scale_, skew);
            return;
        }
        skew = Quat::identity;
    }

    if (trans_) trans_ = getTrans();
    if (rot_) rot_ = rot.inverse(); //Dx mat is transposed, so quat is inversed
    if (scale_) scale_ = scale;
}

}

#endif
#endif