// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Numeral.h"
#include "Honey/Memory/UniquePtr.h"
#include "Honey/String/Bytes.h"

namespace honey
{

/// A compact array of bits. Dynamic version of std::bitset.
template<class Block = uint64, class Alloc_ = std::allocator<Block>>
class BitSet_
{
    typedef typename Alloc_::template rebind<Block>::other Alloc;
public:
    static_assert(std::is_unsigned<Block>::value, "block type must be unsigned");

    /// Construct array with `size` number of bits, each initialized to `val`
    BitSet_(szt size = 0, bool val = false, const Alloc& a = Alloc()) :
        _alloc(a),
        _size(0),
        _blockCount(0),
        _blocks(nullptr, finalize<Block,Alloc>(_alloc))
    {
        if (size) resize(size, val);
    }

    /// Construct from bytes in big-endian bit order (the first index will contain the MSB)
    BitSet_(const Bytes& bs, const Alloc& a = Alloc()) :
        BitSet_(bs.size()*8, false, a)
    {
        szt i = 0;
        for (auto b: bs) for (auto j: range(8)) set(i++, (b >> (7-j)) & 1);
    }
    
    /// Resize array to contain `size` number of bits, with new bits initialized to `val`
    void resize(szt size, bool val = false)
    {
        assert(size >= 0);
        if (size == _size) return;
        //Allocate new blocks
        szt blockCount = size / bitsPerBlock + (size % bitsPerBlock != 0);
        Block* blocks = nullptr;
        if (size)
        {
            blocks = new (_alloc.allocate(blockCount)) Block[blockCount];
            //Copy old blocks
            if (_blockCount) std::copy(_blocks.get(), _blocks.get() + (blockCount < _blockCount ? blockCount : _blockCount), blocks);
        }
        //Init new bits if new array is larger
        sdt blockDif = blockCount - _blockCount;
        if (blockDif >= 0)
        {
            //Init all new blocks
            std::fill_n(blocks + _blockCount, blockDif, val ? ~Block(0) : 0);
            //Init first new bits before new blocks
            Block firstBits = unusedBitsMask();
            if (firstBits) val ? blocks[_blockCount-1] |= firstBits : blocks[_blockCount-1] &= ~firstBits;
        }
        //Assign new array
        _size = size;
        _blockCount = blockCount;
        _blocks = blocks;
        //Zero out unused bits
        trim();
    }

    /// Set bit to true
    void set(szt index)                         { assert(index < size()); _blocks[blockIndex(index)] |= bitMask(index); }
    /// Set bit to value
    void set(szt index, bool val)               { val ? set(index) : reset(index); }
    /// Set all bits to true
    void set()                                  { if (!_blockCount) return; std::fill_n(_blocks.get(), _blockCount, ~Block(0)); trim(); }
    /// Set bit to false
    void reset(szt index)                       { assert(index < size()); _blocks[blockIndex(index)] &= ~bitMask(index); }
    /// Set all bits false
    void reset()                                { if (!_blockCount) return; std::fill_n(_blocks.get(), _blockCount, 0); }
    /// Flip value of bit
    void flip(szt index)                        { assert(index < size()); _blocks[blockIndex(index)] ^= bitMask(index); }
    /// Flip values of all bits
    void flip()                                 { for (szt i = 0; i < _blockCount; ++i) _blocks[i] = ~_blocks[i]; trim(); }
    /// Get bit value
    bool test(szt index) const                  { assert(index < size()); return _blocks[blockIndex(index)] & bitMask(index); }
    
    /// Test if all bits are true
    bool all() const
    {
        if (!_blockCount) return false;
        for (szt i = 0; i < _blockCount-1; ++i) if (~_blocks[i]) return false;
        return _blocks[_blockCount-1] == ~unusedBitsMask();
    }
    
    /// Test if any bits are true
    bool any() const                            { for (szt i = 0; i < _blockCount; ++i) { if (_blocks[i]) return true; } return false; }
    /// Test if no bits are true
    bool none() const                           { return !any(); }

    /// Count number of true values in bit array
    szt count() const                           { szt count = 0; for (szt i = 0; i < _blockCount; ++i) count += BitOp::popCount(_blocks[i]); return count; }

    /// Number of bits in the bit array
    szt size() const                            { return _size; }
    /// Number of blocks that the bit array has been split up into
    szt blockCount() const                      { return _blockCount; }
    
    static const szt bitsPerBlock               = sizeof(Block)*8;

    /// Get access to raw blocks that hold the bits. Bit index 0 is the LSB of block 0.
    const Block* blocks() const                 { return _blocks; }
    Block* blocks()                             { return _blocks; }
    
    /// Create Bytes from big-endian bits (the first index contains the MSB)
    Bytes toBytes() const
    {
        Bytes bs;
        bs.resize(size()/8 + (size()%8 != 0));
        szt i = 0;
        for (auto& b: bs) for (auto j: range(8)) if (i < size()) b |= test(i++) << (7-j);
        return bs;
    }
    
private:
    static const szt bitToBlockShift            = mt::log2Floor<bitsPerBlock>::value;
    static const szt bitOffsetMask              = bitsPerBlock-1;

    static szt blockIndex(szt index)            { return index >> bitToBlockShift; }
    static Block bitMask(szt index)             { return Block(1) << (index & bitOffsetMask); }

    /// Get mask for unused bits in last block
    Block unusedBitsMask() const                { szt bits = _size % bitsPerBlock; return bits ? ~((Block(1) << bits) - 1) : 0; }
    /// It is convenient to always have the unused bits in last block be 0
    void trim()                                 { if (!_blockCount) return; _blocks[_blockCount-1] &= ~unusedBitsMask(); }

    Alloc _alloc;
    szt _size;
    szt _blockCount;
    UniquePtr<Block, finalize<Block,Alloc>> _blocks;
};

typedef BitSet_<> BitSet;

}
