// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"

namespace honey
{

/// A 3D linear transform from TRS components (translation, rotation, and scale/skew)
/**
  * The TRS model is equivalent to a 4x4 affine homogeneous matrix where translation occupies the last column,
  * and rotation combines with scale/skew to form a 3x3 sub-matrix in the upper-left:
  *
  *     | RS   RS   RS   Tx |   T: Translation
  *     | RS   RS   RS   Ty |   R: Rotation
  *     | RS   RS   RS   Tz |   S: Scale/Skew
  *     | 0    0    0    1  |
  *
  * \f$ tm = T R S \f$      ->  A transform first scales (_S_), then rotates (_R_), then translates (_T_). \n
  * \f$ S = U K U^{-1} \f$  ->  Scaling is done by first rotating into scale-space using the inverse of skew (\f$U^{-1}\f$)
  *                             then scaling (_K_), then rotating back out of scale-space (_U_).
  *
  * Advantages of TRS model over affine matrix model:
  *     - Rotation, scale and skew can be accessed independently and immediately
  *     - Quats can be used directly for rotation and skew (don't need to convert to 3x3 rot matrix)
  *     - Faster than matrices at forward and inverse transform ops (unless matrices have hardware acceleration)
  *
  * Disadvantages:
  *     - Can't concatenate skews
  *     - Can't represent projective transforms (ie. where last row in matrix is not identity)
  *
  * Be wary of the following operations:
  *     - A*B where A has non-uniform scale and B has rotation                  -> returns transform with skew
  *     - A.inverse() where A has non-uniform scale and rotation                -> returns transform with skew
  *     - A*B where A and B both have non-uniform scale and B has rotation      -> error
  *     - A*B where A and B both have non-uniform scale and either has skew     -> error
  *     - A*B where A and B both have skew                                      -> error
  */
template<class Real>
class Transform_
{
protected:
    typedef Vec<2,Real>         Vec2;
    typedef Vec<3,Real>         Vec3;
    typedef Vec<4,Real>         Vec4;
    typedef Matrix<4,4,Real>    Matrix4;
    typedef Quat_<Real>         Quat;

public:
    /// Init to identity
    Transform_()                                                        { fromIdentity(); }

    /// Construct from TRS components
    Transform_( const Vec3& trans,              const Quat& rot = Quat::identity,
                const Vec3& scale = Vec3::one,  const Quat& skew = Quat::identity)
                                                                        { fromTrs(trans, rot, scale, skew); }

    /// Construct from matrix.  Matrix will be decomposed, an expensive operation.
    explicit Transform_(const Matrix4& mat)                             { fromMatrix(mat); }

    ~Transform_()                                                       {}

    /// Init to identity
    Transform_& fromIdentity()
    {
        resetTrans();
        resetRot();
        resetScale();
        return *this;
    }

    /// Init from TRS components
    Transform_& fromTrs( const Vec3& trans,              const Quat& rot = Quat::identity,
                         const Vec3& scale = Vec3::one,  const Quat& skew = Quat::identity)
    {
        setTrans(trans);
        setRot(rot);
        setScale(scale, skew);
        return *this;
    }

    /// Init from matrix. Matrix will be decomposed, an expensive operation.
    Transform_& fromMatrix(const Matrix4& mat)
    {
        mat.decompose(_trans, _rot, _scale, _skew);
        return fromTrs(_trans, _rot, _scale, _skew);
    }

    Transform_& operator=(const Transform_& rhs)
    {
        _trans = rhs._trans;
        _rot = rhs._rot;
        _scale = rhs._scale;
        _skew = rhs._skew;
        _bTrans = rhs._bTrans;
        _bRot = rhs._bRot;
        _bScale = rhs._bScale;
        _bUniformScale = rhs._bUniformScale;
        _bSkew = rhs._bSkew;
        onTmChange();
        return *this;
    }

    Transform_ operator*(const Transform_& tm) const;
    
    Vec3 operator*(const Vec3& v) const
    {
        Vec3 ret = v;
        if (_bScale)
            ret = _bSkew ? _skew * (_skew.inverse()*ret).elemMul(_scale) : ret.elemMul(_scale);
        if (_bRot)
            ret = _rot * ret;
        ret += _trans;
        return ret;
    }

    Vec4 operator*(const Vec4& v) const                                 { return Vec4(operator*(Vec3(v)), v.w); }
    Vec2 operator*(const Vec2& v) const                                 { return Vec2(operator*(Vec3(v))); }

    Transform_& operator*=(const Transform_& tm)                        { return *this = operator*(tm); }

    /// Transform by just rotation and scale/skew components (no translation)
    Vec3 mulRotScale(const Vec3& v) const
    {
        Vec3 ret = v;
        if (_bScale)
            ret = _bSkew ? _skew * (_skew.inverse()*ret).elemMul(_scale) : ret.elemMul(_scale);
        if (_bRot)
            ret = _rot * ret;
        return ret;
    }

    bool operator == (const Transform_& rhs) const                      { return _trans == rhs._trans && _rot == rhs._rot && _scale == rhs._scale; }
    bool operator != (const Transform_& rhs) const                      { return !operator==(rhs); }

    Transform_ inverse() const;

    void resetTrans()                                                   { setTrans(Vec3::zero); }
    void resetRot()                                                     { setRot(Quat::identity); }
    void resetScale()                                                   { setScale(Vec3::one); }

    void setTrans(const Vec3& trans)
    {
        _trans = trans;
        _bTrans = _trans != Vec3::zero;
        onTmChange();
    }

    const Vec3& getTrans() const                                        { return _trans; }

    void setRot(const Quat& rot)
    {
        _rot = rot;
        _bRot = _rot != Quat::identity;
        onTmChange();
    }

    const Quat& getRot() const                                          { return _rot; }
    
    void setScale(const Vec3& scale, const Quat& skew = Quat::identity)
    {
        _skew = skew;
        _bSkew = _skew != Quat::identity;
        _scale = scale;
        _bScale = _scale != Vec3::one || _bSkew;
        _bUniformScale = _scale.x == _scale.y && _scale.x == _scale.z && !_bSkew;
        onTmChange();
    }

    void setScale(Real f)                                               { setScale(Vec3(f)); }

    const Vec3& getScale() const                                        { return _scale; }
    const Quat& getSkew() const                                         { return _skew; }

    void getTrs(optional<Vec3&> trans = optnull, optional<Quat&> rot = optnull,
                optional<Vec3&> scale = optnull, optional<Quat&> skew = optnull) const
    {
        if (trans) trans = _trans;
        if (rot) rot = _rot;
        if (scale) scale = _scale;
        if (skew) skew = _skew;
    }

    /// Make a tm that performs this transform first, then does a translation. ie. T * this
    Transform_& translate(const Vec3& v)                                { Transform_ tm; tm.setTrans(v); return *this = tm * *this; }
    /// Make a tm that does a translation first, then performs this transform. ie. this * T
    Transform_& preTranslate(const Vec3& v)                             { Transform_ tm; tm.setTrans(v); return operator*=(tm);  }

    /// Make a tm that performs this transform first, then does a rotation. ie. R * this
    Transform_& rotate(const Quat& q)                                   { Transform_ tm; tm.setRot(q); return *this = tm * *this; }
    /// Make a tm that does a rotation first, then performs this transform. ie. this * R
    Transform_& preRotate(const Quat& q)                                { Transform_ tm; tm.setRot(q); return operator*=(tm); }

    /// Make a tm that performs this transform first, then does a scale. ie. S * this
    Transform_& scale(const Vec3& v, const Quat& skew = Quat::identity) { Transform_ tm; tm.setScale(v, skew); return *this = tm * *this; }
    /// Uniform scale
    Transform_& scale(Real f)                                           { return scale(Vec3(f)); }

    /// Make a tm that does a scale first, then performs this transform. ie. this * S
    Transform_& preScale(const Vec3& v, const Quat& skew = Quat::identity)  { Transform_ tm; tm.setScale(v, skew); return operator*=(tm); }
    /// Uniform prescale
    Transform_& preScale(Real f)                                        { return preScale(Vec3(f)); }

    bool isIdentity() const                                             { return !_bTrans && !_bRot && !_bScale; }
    bool hasTrans() const                                               { return _bTrans; }
    bool hasRot() const                                                 { return _bRot; }
    bool hasScale() const                                               { return _bScale; }
    bool hasUniformScale() const                                        { return _bUniformScale; }
    bool hasSkew() const                                                { return _bSkew; }

    friend ostream& operator<<(ostream& os, const Transform_& tm)
    {
        return os << "{ "   << "trans: " << tm._trans << ", rot: " << tm._rot
                            << ", scale: " << tm._scale << ", skew: " << tm._skew << " }";
    }

protected:
    virtual void onTmChange()                                           {}

private:
    /// Constructor without init to identity
    Transform_(mt::tag<0>)                                              {}

    Vec3                _trans;
    Quat                _rot;
    Vec3                _scale;
    Quat                _skew;

    bool                _bTrans;
    bool                _bRot;
    bool                _bScale;
    bool                _bUniformScale;
    bool                _bSkew;

public:
    static const Transform_ identity;
};

template<class Real> const Transform_<Real> Transform_<Real>::identity;

typedef Transform_<Real>   Transform;
typedef Transform_<Float>  Transform_f;
typedef Transform_<Double> Transform_d;

extern template class Transform_<Float>;
extern template class Transform_<Double>;

}