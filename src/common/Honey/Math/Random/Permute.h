// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Random.h"

namespace honey
{

/// Generate all permutations of a list.  A functor can be specified for fast culling of entire subtrees of undesired permutations. 
/**
  * The permuations are in lexicographic order. ie. {1,2,3} , {1,3,2} , {2,1,3} , {2,3,1} , {3,1,2} , {3,2,1} \n
  * The current permutation list can be accessed with the dereference/pointer operators.
  *
  * If a functor is provided then undesired subtrees can be culled. For example, skip all permutations that start with {0,1}:
  * 
  *     Permute::range(list, [](const vector<const Real*>& perm) { return !(perm.size() == 2 && *perm[0] == 0 && *perm[1] == 1); });
  *
  * When an iter is stepped (++), its functor will be called before traversing each permutation subtree. \n
  * For example, `perm` will first contain {1}, if the functor returns true then the next test is {1,2}, then finally {1,2,3}. \n
  * When the functor returns true for a full permutation (ex. {1,2,3}), then the iter step is complete.
  *
  * Copies of the iter share its permutation state, so a change to one iter affects all others.
  *
  * Algorithm from: "Lexicographic Permutations with Restricted Prefixes" from The Art of Computer Programming, Vol 4, Section 7.2.1.2
  */
template<class Real>
class Permute_
{
    typedef typename Numeral<Real>::Real_   Real_;
    typedef typename Real_::DoubleType      Double_;
    typedef typename Double_::Real          Double;
    typedef Alge_<Real>                     Alge;
    
public:

    /// Iter for permutations of a list
    template<class T>
    class Iter
    {
        friend class Permute_;
        typedef function<bool (const vector<const T*>&)> Func;

        struct State : public SharedObj<State>
        {
            State(const vector<T>& list, const Func& func = nullptr)
                                                        : list(list), func(func), count(0), countMax(0), k(-1), n(0) {}

            const vector<T>& list;
            Func func;
            vector<const T*> perm;
            sdt count;
            sdt countMax;

            vector<sdt> a;
            vector<sdt> l;
            vector<sdt> u;
            sdt p;
            sdt q;
            sdt k;
            sdt n;
        };

    public:
        typedef std::forward_iterator_tag               iterator_category;
        typedef const vector<const T*>                  value_type;
        typedef sdt                                     difference_type;
        typedef const vector<const T*>*                 pointer;
        typedef const vector<const T*>&                 reference;
        
        Iter() = default;
        Iter(SharedPtr<State> state);

        Iter& operator++();
        Iter operator++(int)                            { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const Iter& rhs) const;
        bool operator!=(const Iter& rhs) const          { return !operator==(rhs); }

        /// Get current permutation list
        vector<const T*>& operator*() const             { return _ps->perm; }
        vector<const T*>* operator->() const            { return &_ps->perm; }

        /// Get current permutation number. Every perm has a unique associated number: the first perm is 1, the last perm is countMax()
        sdt count() const                               { return _ps->count; }
        /// Get total number of permutations for this list
        sdt countMax() const                            { return _ps->countMax; }
        
    private:
        bool isEnd() const                              { return !_ps; }

        SharedPtr<State> _ps;
    };

    /// Create a permutation range
    template<class T>
    static Range_<Iter<T>, Iter<T>>
        range(const vector<T>& list, const typename Iter<T>::Func& func = nullptr)
                                                        { return honey::range(Iter<T>(new typename Iter<T>::State(list, func)), Iter<T>(nullptr)); }
};

template<class Real>
template<class T>
Permute_<Real>::Iter<T>::Iter(SharedPtr<State> state) :
    _ps(state)
{
    if (isEnd() || _ps->list.size() == 0)
        return;

    _ps->n = _ps->list.size();
    _ps->countMax = GammaFunc_<Double>::factorial(_ps->n);
    _ps->count = 0;
    _ps->a.resize(_ps->n+1, -1);
    _ps->l.resize(_ps->n+1, -1);
    _ps->u.resize(_ps->n+1, -1);

    for (sdt i = 0; i < _ps->n; ++i)
        _ps->l[i] = i+1;
    _ps->l[_ps->n] = 0;
    _ps->k = 1;
    //Init level k
    _ps->p = 0;
    _ps->q = _ps->l[0];
    operator++();
}

template<class Real>
template<class T>
auto Permute_<Real>::Iter<T>::operator++() -> Iter&
{
    if (_ps->k <= 0)
    {
        if (_ps->k == 0)
            --_ps->k;  //Put iterator into end state
        return *this;
    }

    while(1)
    {
        //Call functor to test for level k culling
        bool visit = false;
        bool func = true;

        _ps->a[_ps->k] = _ps->q;

        //Only call functor if we're using one, otherwise assume it returned true
        if (_ps->func)
        {
            //Build permutation up to level k
            _ps->perm.resize(_ps->k);
            for (sdt i = 1; i <= _ps->k; ++i)
                _ps->perm[i-1] = &_ps->list[_ps->a[i]-1];
            //Call functor for cull test
            func = _ps->func(const_cast<const vector<const T*>&>(_ps->perm));
        }

        if (func && _ps->k < _ps->n)
        {
            //Functor true, not full permutation. Advance and init level k
            _ps->u[_ps->k] = _ps->p;
            _ps->l[_ps->p] = _ps->l[_ps->q];
            ++_ps->k;
            _ps->p = 0;
            _ps->q = _ps->l[0];
            continue;
        }
        else if (func)
        {
            //Functor true, full permutation. Visit
            visit = true;
            _ps->count++;

            //If we're using a functor then the full permutation has already been built and is ready for visiting
            if (!_ps->func)
            {
                _ps->perm.resize(_ps->k);
                for (sdt i = 1; i <= _ps->k; ++i)
                    _ps->perm[i-1] = &_ps->list[_ps->a[i]-1];
            }
        }
        else
        {
            //Functor false, skip subtree. Increase a[k]
            _ps->p = _ps->q;
            _ps->q = _ps->l[_ps->p];
            _ps->count = _ps->count + GammaFunc_<Double>::factorial(_ps->n - _ps->k);
            if (_ps->q != 0)
                continue;
        }

        while (1)
        {
            //Fallback levels. Decrease k
            --_ps->k;
            if (_ps->k <= 0)
                return *this;
            _ps->p = _ps->u[_ps->k];
            _ps->q = _ps->a[_ps->k];
            _ps->l[_ps->p] = _ps->q;

            //Increase a[k]
            _ps->p = _ps->q;
            _ps->q = _ps->l[_ps->p];
            if (_ps->q != 0)
                break;
        }

        if (visit)
            break;
    }

    return *this;
}

template<class Real>
template<class T>
bool Permute_<Real>::Iter<T>::operator==(const Iter& rhs) const
{
    if (!isEnd() && !rhs.isEnd())
        return _ps == rhs._ps;
    if (isEnd() && rhs.isEnd())
        return true;

    //Comparing to end, check if done
    const Iter& it = isEnd() ? rhs : *this;
    return it._ps->k < 0;
}


typedef Permute_<Real>       Permute;
typedef Permute_<Float>      Permute_f;
typedef Permute_<Double>     Permute_d;

}

