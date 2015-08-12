// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Memory/Pool.h"
#include "Honey/Math/Alge/Alge.h"

namespace honey
{

void MemPool::Bucket::initChunk(uint8* chunk, int chunkSize, int blockCount)
{
    //Align first block
    uint8* blockData = alignCeil(chunk + sizeof(BlockHeader), _pool._blockAlign);
    BlockHeader* first = blockHeader(blockData);
    BlockHeader* prev = nullptr;
    //Initialize and link all the new blocks in order
    for (auto i : range(blockCount))
    {
        mt_unused(i);
        BlockHeader* header = blockHeader(blockData);
        header->bucket = _bucketIndex;
        header->offset = 0;
        #ifdef DEBUG
            header->debug.size = 0;
            header->debug.srcFile = nullptr;
            header->debug.srcLine = 0;
            header->debug.prev = nullptr;
            header->debug.sig = BlockHeader::sigFree;
        #endif
        header->next = nullptr;

        if (prev) prev->next = header;
        prev = header;
        blockData += blockStride();
    }

    //Attach free blocks to tail
    if (_freeTail) _freeTail->next = first;
    _freeTail = prev;
    if (!_freeHead) _freeHead = first;

    //Add memory chunk to list so it can be freed
    _chunkList.push_back(chunk);
    _chunkSizeTotal += chunkSize;
    _freeCount += blockCount;
}

void* MemPool::Bucket::alloc(int size, int align_, const char* srcFile, int srcLine)
{
    mt_unused(size); mt_unused(srcFile); mt_unused(srcLine);

    BlockHeader* header;
    {
        //Detach head free block
        //Lock both head and tail if they are sharing pointers
        SpinLock::Scoped _(_lock);
        SpinLock::Scoped __(_tailLock, _freeCount <= 1 ? lock::Op::lock : lock::Op::defer);
        if (_freeCount == 0) expand();
        assert(_freeHead);
        header = _freeHead;
        _freeHead = header->next;
        if (header == _freeTail)
        {
            assert(_freeCount == 1 && !_freeHead, "Free list double-lock algorithm is broken");
            _freeTail = nullptr;    //Free head/tail have now been consumed
        }
        --_freeCount;
    }

    header->validate(BlockHeader::sigFree);
    //If the current block has offset from alignment, or alignment is requested
    if (header->offset != 0 || align_ > 1)
    {
        //Set up header behind aligned block
        //The current block may have alignment applied
        //We want to revert to the original (no offset), then align the original
        uint8* original = blockData(header) - header->offset;
        uint8* aligned = alignCeil(original, align_);
        //Erase header
        #ifdef DEBUG
            header->debug.sig = 0;
        #endif
        //Init new header
        header = blockHeader(aligned);
        header->bucket = _bucketIndex;
        header->offset = aligned - original;
    }

    #ifdef DEBUG
        header->debug.sig = BlockHeader::sigUsed;
        header->debug.size = size;
        header->debug.srcFile = srcFile;
        header->debug.srcLine = srcLine;

        //Attach block to used list as head
        {
            SpinLock::Scoped _(_lock);
            if (_usedHead) _usedHead->debug.prev = header;
            header->next = _usedHead;
            header->debug.prev = nullptr;
            _usedHead = header;
            _usedSize += header->debug.size;
        }
    #else
        header->next = nullptr;
    #endif
    ++_usedCount;

    return blockData(header);
}

void MemPool::Bucket::expand()
{
    /// Assumes that bucket is locked
    int expandCount = (_freeCount + _usedCount) / 2 + 1; //Expand 50%
    int allocSize = blockOffsetMax() + blockStride() * expandCount;
    uint8* chunk = honey::alloc<uint8>(allocSize);
    assert(chunk, sout() << "Allocation failed: " << allocSize << " bytes");
    initChunk(chunk, allocSize, expandCount);
}

void MemPool::Bucket::free(BlockHeader* header)
{
    header->validate(BlockHeader::sigUsed);

    #ifdef DEBUG
        {
            //Detach from used list
            SpinLock::Scoped _(_lock);
            if (_usedHead == header) _usedHead = header->next;
            if (header->debug.prev) header->debug.prev->next = header->next;
            if (header->next) header->next->debug.prev = header->debug.prev;
            _usedSize -= header->debug.size;
        }
        header->debug.sig = BlockHeader::sigFree;
    #endif
    --_usedCount;

    header->next = nullptr;
    {
        //Attach to free list as tail
        SpinLock::Scoped _(_tailLock);
        if (_freeCount == 0)
        {
            assert(!_freeHead && !_freeTail, "Free list double-lock algorithm is broken");
            _freeHead = header;
        }
        if (_freeTail) _freeTail->next = header;
        _freeTail = header;
        ++_freeCount;
    }
}

void* MemPool::Heap::alloc(int size, int align_, const char* srcFile, int srcLine)
{
    mt_unused(srcFile); mt_unused(srcLine);

    int alignSize = align_-1 + sizeof(BlockHeader) + size;
    BlockHeader* headerUnalign = reinterpret_cast<BlockHeader*>(honey::alloc<uint8>(alignSize));
    BlockHeader* header = blockHeader(alignCeil(blockData(headerUnalign), align_));
    header->offset = reinterpret_cast<uint8*>(header) - reinterpret_cast<uint8*>(headerUnalign);
    header->size = alignSize;
    header->heap = this;

    #ifdef DEBUG
        header->debug.size = alignSize;
        header->debug.srcFile = srcFile;
        header->debug.srcLine = srcLine;
        header->debug.sig = BlockHeader::sigUsed;
        {
            //Attach block to used list as head
            SpinLock::Scoped _(_lock);
            if (_usedHead) _usedHead->debug.prev = header;
            header->next = _usedHead;
            header->debug.prev = nullptr;
            _usedHead = header; 
        }
        ++_usedCount;
    #endif

    //Add to total allocated
    _chunkSizeTotal += header->size;

    return blockData(header);
}

void MemPool::Heap::free(BlockHeader* header)
{
    header->validate(BlockHeader::sigUsed);

    #ifdef DEBUG
        {
            //Detach from used list
            SpinLock::Scoped _(_lock);
            if (_usedHead == header) _usedHead = header->next_();
            if (header->debug.prev) header->debug.prev->next = header->next;
            if (header->next) header->next->debug.prev = header->debug.prev;
        }
        header->debug.sig = BlockHeader::sigFree;
        --_usedCount;
    #endif
    
    //Remove from total allocated
    _chunkSizeTotal -= header->size;

    honey::free(reinterpret_cast<uint8*>(header) - header->offset);
}

MemPool::MemPool(const Factory& factory) :
    _blockAlign(factory.param[Factory::align()]),
    _bucketChunk(nullptr)
{
    //Initialize the buckets from the factory
    for (auto& e : factory.bucketList)
    {
        _bucketMap[e[Factory::blockSize()]] = new Bucket(*this, e[Factory::blockSize()], e[Factory::blockCount()]);
    }

    //Build sorted bucket list
    for (auto& e : values(_bucketMap))
    {
        e->_bucketIndex = size(_bucketList);
        _bucketList.push_back(e);
    }

    assert(!_bucketMap.empty());
    _blockSizeMax = _bucketMap.rbegin()->second->_blockSize;

    //Get a chunk size that can hold all buckets
    int chunkSize = 0;
    for (auto& e : _bucketList) { chunkSize += e->blockOffsetMax() + e->blockStride() * e->_blockCountInit; }

    //Allocate initial contiguous memory chunk
    if (chunkSize > 0)
    {
        _bucketChunk = honey::alloc<uint8>(chunkSize);
        assert(_bucketChunk, sout() << "Allocation failed: " << chunkSize << " bytes");
    }
    //Set up the buckets
    uint8* chunk = _bucketChunk;
    for (auto& e : _bucketList)
    {
        int chunkSize = e->blockOffsetMax() + e->blockStride() * e->_blockCountInit;
        e->initChunk(chunk, chunkSize, e->_blockCountInit);
        chunk += chunkSize;
    }

    _heap = new Heap(*this);
}

void* MemPool::alloc(int size, int align, const char* srcFile, int srcLine)
{
    assert(size > 0 && align >= 1 && align <= numeral<uint16>().max());
    int alignSize = align-1 + size;
    if (alignSize <= _blockSizeMax)
        //Small enough to use bucket allocator
        return _bucketMap.lower_bound(alignSize)->second->alloc(alignSize, align, srcFile, srcLine);
    else
        //Too large for any bucket, use heap allocator
        return _heap->alloc(size, align, srcFile, srcLine);
}

void MemPool::free(void* ptr_)
{
    uint8* ptr = static_cast<uint8*>(ptr_);
    //Get type of block
    if (*reinterpret_cast<Heap**>(ptr - sizeof(Heap*)) == _heap)
        //Heap block
        _heap->free(Heap::blockHeader(ptr));
    else
    {
        //Bucket block
        BlockHeader* header = blockHeader(ptr);
        header->validate(BlockHeader::sigUsed);
        _bucketList[header->bucket]->free(header);
    }
}

int MemPool::allocBytes() const
{
    int total = 0;
    for (auto& e : _bucketList) { total += e->_chunkSizeTotal; }
    total += _heap->_chunkSizeTotal;
    return total;
}

int MemPool::usedBytes() const
{
    int total = 0;
    for (auto& e : _bucketList) { total += e->_blockSize*e->_usedCount; }
    total += _heap->_chunkSizeTotal;
    return total;
}

#ifdef DEBUG

void MemPool::validate() const
{
    lock();

    for (auto& e : _bucketList)
    {
        for (auto header = e->_usedHead; header; header = header->next)
            header->validate(BlockHeader::sigUsed);
        for (auto header = e->_freeHead; header; header = header->next)
            header->validate(BlockHeader::sigFree);
    }

    for (auto header = _heap->_usedHead; header; header = header->next_())
        header->validate(BlockHeader::sigUsed);

    unlock();
}

String MemPool::printStats() const
{
    lock();

    ostringstream stream;
    stream.precision(1);
    stream.setf(std::ios::fixed);
    
    int allocTotal = 0;
    int usedSize = 0;
    int usedCount = 0;
    int freeCount = 0;
    for (auto& e : _bucketList)
    {
        allocTotal += e->_chunkSizeTotal;
        usedSize += e->_usedSize;
        usedCount += e->_usedCount;
        freeCount += e->_freeCount;
    }

    allocTotal += _heap->_chunkSizeTotal;
    usedSize += _heap->_chunkSizeTotal;

    stream  << stringstream::indentInc << "{" << endl;

    stream  << "MemPool Id: " << _id << endl
            << "Total Allocated Bytes: " << allocTotal << endl
            << "Total Used Bytes: " << usedSize
                << " (" << Real(usedSize)/allocTotal*100 << "%)" << endl
            << "Block Header Size: " << sizeof(BlockHeader) << endl
            << "Bucket Count: " << _bucketMap.size() << endl
            << "Bucket Blocks Used: " << usedCount << " / " << (freeCount+usedCount)
                << " (" << Real(usedCount)/(freeCount+usedCount)*100 << "%)" << endl;

    int i = 0;
    for (auto& e : _bucketList)
    {
        int blockCount = e->_freeCount + e->_usedCount;
        stream  << "Bucket #" << i++ << endl
                << stringstream::indentInc << "{" << endl
                << "Block Size: " << e->_blockSize << endl
                << "Block Count Expansion: " << blockCount << " / " << e->_blockCountInit
                    << " (" << Real(blockCount)/e->_blockCountInit*100 << "%)" << endl
                << "Allocated Bytes: " << e->_chunkSizeTotal
                    << " (" << Real(e->_chunkSizeTotal)/allocTotal*100 << "%)" << endl
                << "Blocks Used: " << e->_usedCount << " / " << blockCount
                    << " (" << Real(e->_usedCount)/blockCount*100 << "%)" << endl
                << "Avg Block Fill: " << e->_usedSize << " / " << e->_blockSize*e->_usedCount
                    << " (" << (e->_usedSize == 0 ? Real(0) : Real(e->_usedSize)/(e->_blockSize*e->_usedCount)*100) << "%)"
                << stringstream::indentDec << endl << "}" << endl;
    }

    stream  << "Heap " << endl
            << stringstream::indentInc << "{" << endl
            << "Allocated Bytes: " << _heap->_chunkSizeTotal
                << " (" << Real(_heap->_chunkSizeTotal)/allocTotal*100 << "%)" << endl
            << "Blocks Used: " << _heap->_usedCount
            << stringstream::indentDec << endl << "}";

    stream << stringstream::indentDec << endl << "}" << endl;

    unlock();

    return stream;
}

String MemPool::printUsed() const
{
    lock();

    ostringstream stream;
    stream.precision(3);

    int usedCount = 0;
    int usedSize = 0;
    for (auto& e : _bucketList)
    {
        usedCount += e->_usedCount;
        usedSize += e->_usedSize;
    }

    usedCount += _heap->_usedCount;
    usedSize += _heap->_chunkSizeTotal;

    stream  << "MemPool Id: " << _id << endl
            << "Total Used Bytes: " << usedSize << endl
            << "Total Blocks Used: " << usedCount << endl;

    int bucket = 0;
    int block = 0;
    for (auto& e : _bucketList)
    {
        for (auto header = e->_usedHead; header; header = header->next, ++block)
        {
            header->validate(BlockHeader::sigUsed);
            stream  << "Block #" << block << endl
                    << "{" << endl
                    << "    Allocator: Bucket #" << bucket << endl
                    << "    Alloc Size: " << header->debug.size
                        << " (" << Real(header->debug.size)/usedSize*100 << "%)" << endl
                    << "    Source File: " << c_str(header->debug.srcFile) << endl
                    << "    Source Line: " << header->debug.srcLine << endl
                    << "}" << endl;
        }
        ++bucket;
    }

    for (auto header = _heap->_usedHead; header; header = header->next_(), ++block)
    {
        header->validate(BlockHeader::sigUsed);
        stream  << "Block #" << block << endl
                << "{" << endl
                << "    Allocator: Heap" << endl
                << "    Alloc Size: " << header->debug.size
                    << " (" << Real(header->debug.size)/usedSize*100 << "%)" << endl
                << "    Source File: " << c_str(header->debug.srcFile) << endl
                << "    Source Line: " << header->debug.srcLine << endl
                << "}" << endl;
    }

    unlock();

    return stream;
}

#endif //DEBUG

}
