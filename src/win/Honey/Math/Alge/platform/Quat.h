// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#ifdef HONEY_DX9
#ifdef Section_Header

namespace honey
{

template<> Quat_<Float>& Quat_<Float>::fromAxisAngle(const Vec3& axis, Real angle);
template<> Quat_<Float>& Quat_<Float>::fromMatrix(const Matrix4& rot);
template<> Quat_<Float> Quat_<Float>::operator*(const Quat_& rhs) const;
template<> Quat_<Float> Quat_<Float>::exp() const;
template<> Quat_<Float> Quat_<Float>::ln() const;
template<> Quat_<Float> Quat_<Float>::normalize(option<Real&> len) const;
template<> Quat_<Float> Quat_<Float>::normalize_fast() const { return normalize(); }
template<> void Quat_<Float>::axisAngle(Vec3& axis, Real& angle) const;
template<> Quat_<Float>::Matrix4& Quat_<Float>::toMatrix(Matrix4& rot, bool b3x3) const;
template<> Quat_<Float> Quat_<Float>::slerp_fast(Real t, const Quat_& q0, const Quat_& q1, Real cosAlpha);

template<> void Quat_<Float>::squadSetup( const Quat_& q0, const Quat_& q1, const Quat_& q2, const Quat_& q3,
                                                Quat_& a, Quat_& b, Quat_& c);

template<> Quat_<Float> Quat_<Float>::squad(Real t, const Quat_& q1, const Quat_& a, const Quat_& b, const Quat_& c);
template<> Quat_<Float> Quat_<Float>::baryCentric(Real f, Real g, const Quat_& q0, const Quat_& q1, const Quat_& q2);



}

#endif

#ifdef Section_Source

namespace honey
{

static FLOAT*                   DX(Float& f)                { return reinterpret_cast<FLOAT*>(&f); }
static D3DXVECTOR3*             DX(Vec3_f& v)               { return reinterpret_cast<D3DXVECTOR3*>(&v); }
static const D3DXVECTOR3*       DX(const Vec3_f& v)         { return reinterpret_cast<const D3DXVECTOR3*>(&v); }
static D3DXQUATERNION*          DX(Quat_f& q)               { return reinterpret_cast<D3DXQUATERNION*>(&q); }
static const D3DXQUATERNION*    DX(const Quat_f& q)         { return reinterpret_cast<const D3DXQUATERNION*>(&q); }
static D3DXMATRIX*              DX(Matrix4_f& m)            { return reinterpret_cast<D3DXMATRIX*>(&m); }
static const D3DXMATRIX*        DX(const Matrix4_f& m)      { return reinterpret_cast<const D3DXMATRIX*>(&m); }


template<> Quat_<Float>& Quat_<Float>::fromAxisAngle(const Vec3& axis, Real angle)
{
    D3DXQuaternionRotationAxis(DX(*this), DX(axis), angle);
    return *this;
}

template<> Quat_<Float>& Quat_<Float>::fromMatrix(const Matrix4& rot)
{
    D3DXQuaternionRotationMatrix(DX(*this), DX(rot));
    x = -x; y = -y; z = -z; //dx mat is transposed, so quat from dx is inversed
    return *this;
}

template<> Quat_<Float> Quat_<Float>::operator*(const Quat_& rhs) const
{
    Quat_ ret;
    D3DXQuaternionMultiply(DX(ret), DX(rhs), DX(*this));
    return ret;
}

template<> Quat_<Float> Quat_<Float>::exp() const
{
    Quat_ ret;
    D3DXQuaternionExp(DX(ret), DX(*this));
    return ret;
}

template<> Quat_<Float> Quat_<Float>::ln() const
{
    Quat_ ret;
    D3DXQuaternionLn(DX(ret), DX(*this));
    return ret;
}

template<> Quat_<Float> Quat_<Float>::normalize(option<Real&> len) const
{
    if (len)
    {
        Real l = length();
        if (l > RealT::zeroTol)
        {
            len = l;
            return *this / l;
        }
        len = 0;
        return zero;
    }

    Quat_ ret;
    D3DXQuaternionNormalize(DX(ret), DX(*this));
    return ret;
}

template<> void Quat_<Float>::axisAngle(Vec3& axis, Real& angle) const
{
    D3DXQuaternionToAxisAngle(DX(*this), DX(axis), DX(angle));
    axis = axis.normalize();
}

template<> Quat_<Float> Quat_<Float>::slerp_fast(Real t, const Quat_& q0, const Quat_& q1, Real /*cosAlpha*/)
{
    Quat_ ret;
    D3DXQuaternionSlerp(DX(ret), DX(q0), DX(q1), t);
    return ret;
}

template<> Quat_<Float>::Matrix4& Quat_<Float>::toMatrix(Matrix4& rot, bool b3x3) const
{
    Quat_ inv = inverse(); //Dx mat is transposed, must use inverse

    if (!b3x3)
    {
        D3DXMatrixRotationQuaternion(DX(rot), DX(inv));  
        return rot;
    }

    //Dx overwrites entire matrix.  Save values outside of upper-left 3x3 submatrix.
    const Real save[] = {   rot( 3), rot( 7), rot(11),
                            rot(12), rot(13), rot(14), rot(15)};

    D3DXMatrixRotationQuaternion(DX(rot), DX(inv));
    
    //Restore values
    rot( 3) = save[0]; rot( 7) = save[1]; rot(11) = save[2];
    rot(12) = save[3]; rot(13) = save[4]; rot(14) = save[5]; rot(15) = save[6];
    
    return rot;
}

template<> void Quat_<Float>::squadSetup(   const Quat_& q0, const Quat_& q1, const Quat_& q2, const Quat_& q3,
                                            Quat_& a, Quat_& b, Quat_& c)
{
    D3DXQuaternionSquadSetup(DX(a), DX(b), DX(c), DX(q0), DX(q1), DX(q2), DX(q3));
}

template<> Quat_<Float> Quat_<Float>::squad(Float t, const Quat_& q1, const Quat_& a, const Quat_& b, const Quat_& c)
{
    t = Alge::clamp(t, 0, 1);
    Quat_ ret;
    D3DXQuaternionSquad(DX(ret), DX(q1), DX(a), DX(b), DX(c), t);
    return ret;
}

template<> Quat_<Float> Quat_<Float>::baryCentric(Float f, Float g, const Quat_& q0, const Quat_& q1, const Quat_& q2)
{
    f = Alge::clamp(f, 0, 1);
    g = Alge::clamp(g, 0, 1);
    Quat_ ret;
    D3DXQuaternionBaryCentric(DX(ret), DX(q0), DX(q1), DX(q2), f, g);
    return ret;
}

}

#endif
#endif