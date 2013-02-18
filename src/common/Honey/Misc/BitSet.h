// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Numeral.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{

/** \cond */
namespace bitset { namespace priv
{
    extern const int countTable[];
} }
/** \endcond */

/// A compact array of bits. Dynamic version of std::bitset.
template<class Block = uint32, class Alloc_ = std::allocator<Block>>
class BitSet
{
    typedef typename Alloc_::template rebind<Block>::other Alloc;
public:
    static_assert(std::is_unsigned<Block>::value, "block type must be unsigned");

    /// Construct array with `size` number of bits, each initialized to initVal
    BitSet(int size = 0, bool initVal = false, const Alloc& a = Alloc()) :
        _alloc(a),
        _size(0),
        _blockCount(0),
        _bits(nullptr, finalize<Block,Alloc>(_alloc))
    {
        resize(size, initVal);
    }

    /// Resize array to contain `size` number of bits, with new bits initialized to initVal
    void resize(int size, bool initVal = false)
    {
        assert(size >= 0);
        if (size == _size) return;
        //Allocate new blocks
        int blockCount = size / bitsPerBlock + (size % bitsPerBlock != 0);
        Block* bits = nullptr;
        if (size)
        {
            bits = new (_alloc.allocate(blockCount)) Block[blockCount];
            //Copy old blocks
            if (_blockCount) std::copy(_bits.get(), _bits.get() + (blockCount < _blockCount ? blockCount : _blockCount), bits);
        }
        //Init new bits if new array is larger
        int blockDif = blockCount - _blockCount;
        if (blockDif >= 0)
        {
            //Init all new blocks
            std::fill_n(bits + _blockCount, blockDif, initVal ? ~Block(0) : 0);
            //Init first new bits before new blocks
            Block firstBits = unusedBitsMask();
            if (firstBits) initVal ? bits[_blockCount-1] |= firstBits : bits[_blockCount-1] &= ~firstBits;
        }
        //Assign new array
        _size = size;
        _blockCount = blockCount;
        _bits = bits;
        //Zero out unused bits
        trim();
    }

    /// Set bit to true
    void set(int index)                         { assert(index < size()); _bits[blockIndex(index)] |= bitMask(index); }
    /// Set bit to value
    void set(int index, bool val)               { val ? set(index) : reset(index); }
    /// Set all bits to true
    void set()                                  { if (!_blockCount) return; std::fill_n(_bits.get(), _blockCount, ~Block(0)); trim(); }
    /// Set bit to false
    void reset(int index)                       { assert(index < size()); _bits[blockIndex(index)] &= ~bitMask(index); }
    /// Set all bits false
    void reset()                                { if (!_blockCount) return; std::fill_n(_bits.get(), _blockCount, 0); }
    /// Flip value of bit
    void flip(int index)                        { assert(index < size()); _bits[blockIndex(index)] ^= bitMask(index); }
    /// Flip values of all bits
    void flip()                                 { for (int i = 0; i < _blockCount; ++i) _bits[i] = ~_bits[i]; trim(); }
    /// Get bit value
    bool test(int index) const                  { assert(index < size()); return (_bits[blockIndex(index)] & bitMask(index)) != 0; }
    
    /// Test if all bits are true
    bool all() const
    {
        if (!_blockCount) return false;
        for (int i = 0; i < _blockCount-1; ++i) if (~_bits[i]) return false;
        return _bits[_blockCount-1] == ~unusedBitsMask();
    }
    
    /// Test if any bits are true
    bool any() const                            { for (int i = 0; i < _blockCount; ++i) { if (_bits[i]) return true; } return false; }
    /// Test if no bits are true
    bool none() const                           { return !any(); }

    /// Count number of true values in bit array
    int count() const
    {
        int count = 0;
        for (int i = 0; i < _blockCount; ++i)
        {
            Block block = _bits[i];
            for (int j = 0; j < sizeof(Block); ++j)
                count += bitset::priv::countTable[(block >> (j<<3)) & 0xFF];
        }
        return count;
    }

    /// Number of bits in the bit array
    int size() const                            { return _size; }
    /// Number of blocks that the bit array has been split up into
    int blockCount() const                      { return _blockCount; }
    
    static const int bitsPerBlock               = sizeof(Block)*8;

    /// Get access to raw bits in array.  Bit index 0 is the LSB of block 0
    const Block* bits() const                   { return _bits; }

private:
    static const int bitToBlockShift            = mt::log2Floor<bitsPerBlock>::value;
    static const int bitOffsetMask              = bitsPerBlock-1;

    static int blockIndex(int index)            { return index >> bitToBlockShift; }
    static Block bitMask(int index)             { return 1 << (index & bitOffsetMask); }

    /// Get mask for unused bits in last block
    Block unusedBitsMask() const                { int bits = _size % bitsPerBlock; return bits ? ~((1 << bits) - 1) : 0; }
    /// It is convenient to always have the unused bits in last block be 0
    void trim()                                 { if (!_blockCount) return; _bits[_blockCount-1] &= ~unusedBitsMask(); }

    Alloc _alloc;
    int _size;
    int _blockCount;
    UniquePtr<Block, finalize<Block,Alloc>> _bits;
};

}
