// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/BitSet.h"
#include "Honey/String/Id.h"
#include "Honey/Math/Alge/Alge.h"

namespace honey
{

/// BloomFilter util
namespace bloom_filter
{
    /** \cond */
    namespace priv
    {
        extern const szt seedCount;
        extern const uint32 seeds[];
    }
    /** \endcond */

    /// Functor used to generate hash.  Each hashIndex for the same object must produce a unique hash.
    template<class T>
    struct hash
    {
        int operator()(const T& val, szt hashIndex) const       { return honey::hash::fast(ByteBufConst(reinterpret_cast<const byte*>(&val), sizeof(T)), priv::seeds[hashIndex]); }
    };

    /// Calculate optimal bloom parameters -- bit and hash count
    inline tuple<szt,szt> calcParams(szt elemCount, Double errorProb)
    {
        // Optimal hash count (k) formula that minimizes error (p):
        // m = -n*ln(p) / ln(2)^2
        szt bitCount = Alge_d::ceil(-Double(elemCount) * Alge_d::log(errorProb) / Alge_d::sqr(Alge_d::log(2)));
        // k = ln(2) * m / n
        szt hashCount = Alge_d::ceil(Alge_d::log(2) * bitCount / elemCount);
        return make_tuple(bitCount, hashCount);
    }
    
    /// Caches multiple hashes of an object.  The key can be inserted into a bloom filter and tested very quickly.
    template<class T, class Alloc = std::allocator<int>>
    struct Key
    {
        Key() {}
        /// Same params as bloom filter, key will cache the required number of hashes
        Key(szt elemCount, Double errorProb = 0.01, const Alloc& a = Alloc()) :
            hashes(get<1>(calcParams(elemCount, errorProb)), 0, a) {}

        bool operator==(const Key& rhs) const                   { return hashes == rhs.hashes; }
        
        /// Generate and cache all the hashes for the object
        void hash(const T& obj)                                 { for (szt i = 0; i < hashes.size(); ++i) hashes[i] = hasher(obj,i); }

        vector<int,Alloc> hashes;
        bloom_filter::hash<T> hasher;
    };

    /** \cond */
    /// Adapter for bloom hasher
    template<class T, class Alloc>
    struct hash<Key<T,Alloc>>
    {
        int operator()(const Key<T,Alloc>& val, szt hashIndex) const
        { assert(hashIndex < val.hashes.size(), "Key does not have enough hashes for bloom filter"); return val.hashes[hashIndex]; }
    };
    /** \endcond */
}

/// A space-efficient probabilistic data structure that is used to test set membership. Can be tuned to use less space at the expense of increased false positive probabilty.
/**
  * At 1% false positive chance each element uses about 9.6 bits, a further 4.8 bits decreases the error chance ten-fold.
  * By comparison unordered_set uses about 20 bytes per element. \n
  * Note that the elements themselves aren't stored in the bloom filter, and further, elements can't be removed once added.
  */
template<class T, class Block = uint64, class Alloc = std::allocator<Block>>
class BloomFilter
{
public:
    typedef BitSet_<Block, Alloc> BitSet;

    /**
      * \param elemCount    Number of elements expected to be inserted into the set
      * \param errorProb    Probability that contains() will return true even though the element hasn't actually been inserted into the set
      * \param a
      */ 
    BloomFilter(szt elemCount, Double errorProb = 0.01, const Alloc& a = Alloc()) :
        _errorProb(errorProb),
        _bits(0, false, a)
    {
        szt bitCount;
        tie(bitCount, _hashCount) = bloom_filter::calcParams(elemCount, errorProb);
        assert(_hashCount <= bloom_filter::priv::seedCount, "Not enough seeds, either try a higher error prob or add more seeds");
        // Round up to nearest power of two so that hash can be converted to index without a modulo
        _bits.resize(BitOp::pow2Ceil(bitCount));
        _bitIndexMask = _bits.size()-1;
    }

    /// Insert element into set
    void insert(const T& obj)
    {
        for (szt i = 0; i < _hashCount; ++i)
            _bits.set(bitIndex(_hasher(obj, i)));
    }

    /// Check if element is in the set, may return false positive
    bool contains(const T& obj) const
    {
        for (szt i = 0; i < _hashCount; ++i)
            if (!_bits.test(bitIndex(_hasher(obj, i)))) return false;
        return true;
    }

    /// Remove all elements from the set
    void clear()                                { _bits.reset(); }

    /// Get false positive probability 
    Double errorProb() const                    { return _errorProb; }
    /// Get underlying bit array
    const BitSet& bits() const                  { return _bits; }

private:
    /// Convert hash to valid index into bit vector
    szt bitIndex(szt hash) const                { return hash & _bitIndexMask; }

    Double _errorProb;
    BitSet _bits;
    szt _bitIndexMask;
    szt _hashCount;
    bloom_filter::hash<T> _hasher;
};

}

/** \cond */
namespace std
{
    /// Adapter for std hasher
    template<class T, class Alloc>
    struct hash<honey::bloom_filter::Key<T,Alloc>>
    {
        size_t operator()(const honey::bloom_filter::Key<T,Alloc>& val) const
        {
            using namespace honey;
            assert(val.hashes.size());
            return val.hashes[0];
        }
    };
}
/** \endcond */
