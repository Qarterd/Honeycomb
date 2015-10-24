// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Memory/Pool.h"
#include "Honey/Math/Alge/Alge.h"

namespace honey
{

void MemPool::Bucket::initChunk(uint8* chunk, szt chunkSize, szt blockCount)
{
    //Align first block
    uint8* blockData = alignCeil(chunk + sizeof(BlockHeader), _pool._blockAlign);
    BlockHeader* first = blockHeader(blockData);
    BlockHeader* prev = nullptr;
    //Initialize and link all the new blocks in order
    for (auto i : range(blockCount))
    {
        BlockHeader* header = blockHeader(blockData + blockStride()*i);
        
        header->handle = Handle(_chunkCount, static_cast<Handle::Int>(i));
        header->next = nullptr;
        header->offset = 0;
        header->bucket() = _bucketIndex;
        #ifdef DEBUG
            header->debug.size = 0;
            header->debug.srcFile = nullptr;
            header->debug.srcLine = 0;
            header->debug.prev = nullptr;
            header->debug.sig = BlockHeader::Debug::sigFree;
        #endif
        
        if (prev) prev->next = header->handle;
        prev = header;
    }

    //keep track of chunk so that handles can reference their chunk by index
    _chunks[_chunkCount++] = Buffer<uint8>(chunk, chunkSize);
    _chunkSizeTotal += chunkSize;
    
    if (prev)
    {
        //Attach chunk as free head
        TaggedHandle old;
        do
        {
            old = _freeHead;
            prev->next = old;
        } while (!_freeHead.cas(TaggedHandle(first->handle, old.nextTag()), old));
        _freeCount += blockCount;
    }
}

void* MemPool::Bucket::alloc(szt size, uint8 align_, const char* srcFile, int srcLine)
{
    mt_unused(size); mt_unused(srcFile); mt_unused(srcLine);
    
    //Detach head free block
    TaggedHandle old;
    BlockHeader* header;
    do
    {
        while (!(old = _freeHead)) expand();
        header = deref(old);
    } while (!_freeHead.cas(TaggedHandle(header->next, old.nextTag()), old));
    --_freeCount;
    header->validate(BlockHeader::Debug::sigFree);
    
    //If the current block has offset from alignment, or alignment is requested
    if (header->offset != 0 || align_ > 1)
    {
        //Set up header behind aligned block
        //The current block may have alignment applied
        //We want to revert to the original (no offset), then align the original
        uint8* original = blockData(header) - header->offset;
        uint8* aligned = alignCeil(original, align_);
        assert(aligned - original < _blockSize, "Alignment too large for block");
        //Init new header
        Handle handle = header->handle;
        #ifdef DEBUG
            header->debug.sig = 0; //erase old sig
        #endif
        header = blockHeader(aligned);
        header->handle = handle;
        header->offset = aligned - original;
        header->bucket() = _bucketIndex;
    }

    #ifdef DEBUG
        header->debug.sig = BlockHeader::Debug::sigUsed;
        header->debug.size = size;
        header->debug.srcFile = srcFile;
        header->debug.srcLine = srcLine;

        //Attach block to used list as head
        {
            SpinLock::Scoped _(_lock);
            if (_usedHead) deref(_usedHead)->debug.prev = header->handle;
            header->next = _usedHead;
            header->debug.prev = nullptr;
            _usedHead = header->handle;
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
    SpinLock::Scoped _(_lock);
    if (_freeCount) return;
    szt expandCount = _usedCount / 2 + 1; //Expand 50%
    szt allocSize = blockOffsetMax() + blockStride() * expandCount;
    uint8* chunk = honey::alloc<uint8>(allocSize);
    assert(chunk, sout() << "Allocation failed: " << allocSize << " bytes");
    initChunk(chunk, allocSize, expandCount);
}

void MemPool::Bucket::free(BlockHeader* header)
{
    header->validate(BlockHeader::Debug::sigUsed);
    #ifdef DEBUG
        {
            //Detach from used list
            SpinLock::Scoped _(_lock);
            if (_usedHead == header->handle) _usedHead = header->next;
            if (header->debug.prev) deref(header->debug.prev)->next = header->next;
            if (header->next) deref(header->next)->debug.prev = header->debug.prev;
            _usedSize -= header->debug.size;
        }
        header->debug.sig = BlockHeader::Debug::sigFree;
    #endif
    --_usedCount;

    //Attach block as free head
    TaggedHandle old;
    do
    {
        old = _freeHead;
        header->next = old;
    } while (!_freeHead.cas(TaggedHandle(header->handle, old.nextTag()), old));
    ++_freeCount;
}

void* MemPool::Heap::alloc(szt size, uint8 align_, const char* srcFile, int srcLine)
{
    mt_unused(srcFile); mt_unused(srcLine);

    szt alignSize = align_-1 + sizeof(BlockHeader) + size;
    BlockHeader* headerUnalign = reinterpret_cast<BlockHeader*>(honey::alloc<uint8>(alignSize));
    BlockHeader* header = blockHeader(alignCeil(blockData(headerUnalign), align_));
    header->offset = reinterpret_cast<uint8*>(header) - reinterpret_cast<uint8*>(headerUnalign);
    header->size = alignSize;
    header->tag() = BlockHeader::heapTag;

    #ifdef DEBUG
        header->debug.srcFile = srcFile;
        header->debug.srcLine = srcLine;
        header->debug.sig = Bucket::BlockHeader::Debug::sigUsed;
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

    _allocTotal += header->size;

    return blockData(header);
}

void MemPool::Heap::free(BlockHeader* header)
{
    header->validate(Bucket::BlockHeader::Debug::sigUsed);
    #ifdef DEBUG
        {
            //Detach from used list
            SpinLock::Scoped _(_lock);
            if (_usedHead == header) _usedHead = header->next;
            if (header->debug.prev) header->debug.prev->next = header->next;
            if (header->next) header->next->debug.prev = header->debug.prev;
        }
        header->debug.sig = Bucket::BlockHeader::Debug::sigFree;
        --_usedCount;
    #endif
    
    _allocTotal -= header->size;

    honey::free(reinterpret_cast<uint8*>(header) - header->offset);
}

MemPool::MemPool(const vector<tuple<szt,szt>>& buckets, const Id& id, szt align) :
    _id(id),
    _blockAlign(align),
    _bucketChunk(nullptr)
{
    //Initialize the buckets
    for (auto& e : buckets) _bucketMap[get<0>(e)] = new Bucket(*this, get<0>(e), get<1>(e));

    //Build sorted bucket list
    for (auto& e : values(_bucketMap))
    {
        e->_bucketIndex = _buckets.size();
        _buckets.push_back(UniquePtr<Bucket>(e));
    }

    assert(!_bucketMap.empty());
    _blockSizeMax = _bucketMap.rbegin()->second->_blockSize;

    //Get a chunk size that can hold all buckets
    szt chunkSize = 0;
    for (auto& e : _buckets) { chunkSize += e->blockOffsetMax() + e->blockStride() * e->_blockCountInit; }

    //Allocate initial contiguous memory chunk
    if (chunkSize)
    {
        _bucketChunk = UniquePtr<uint8>(honey::alloc<uint8>(chunkSize));
        assert(_bucketChunk, sout() << "Allocation failed: " << chunkSize << " bytes");
    }
    //Set up the buckets
    uint8* chunk = _bucketChunk;
    for (auto& e : _buckets)
    {
        szt chunkSize = e->blockOffsetMax() + e->blockStride() * e->_blockCountInit;
        e->initChunk(chunk, chunkSize, e->_blockCountInit);
        chunk += chunkSize;
    }

    _heap = UniquePtr<Heap>(new Heap(*this));
}

void* MemPool::alloc(szt size, uint8 align, const char* srcFile, int srcLine)
{
    assert(size > 0 && align >= 1);
    szt alignSize = align-1 + size;
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
    if (*(ptr - 1) == Heap::BlockHeader::heapTag)
        //Heap block
        _heap->free(Heap::blockHeader(ptr));
    else
    {
        //Bucket block
        Bucket::BlockHeader* header = Bucket::blockHeader(ptr);
        header->validate(Bucket::BlockHeader::Debug::sigUsed);
        assert(header->bucket() < _buckets.size());
        _buckets[header->bucket()]->free(header);
    }
}

szt MemPool::allocBytes() const
{
    szt total = 0;
    for (auto& e : _buckets) { total += e->_chunkSizeTotal; }
    total += _heap->_allocTotal;
    return total;
}

szt MemPool::usedBytes() const
{
    szt total = 0;
    for (auto& e : _buckets) { total += e->_blockSize*e->_usedCount; }
    total += _heap->_allocTotal;
    return total;
}

#ifdef DEBUG

void MemPool::lock() const
{
    for (auto& e : _buckets) e->_lock.lock();
    _heap->_lock.lock();
}

void MemPool::unlock() const
{
    for (auto& e : _buckets) e->_lock.unlock();
    _heap->_lock.unlock();
}
    
void MemPool::validate() const
{
    lock();

    for (auto& e : _buckets)
    {
        for (Bucket::Handle handle = e->_usedHead; handle; handle = e->deref(handle)->next)
            e->deref(handle)->validate(Bucket::BlockHeader::Debug::sigUsed);
        for (Bucket::Handle handle = e->_freeHead; handle; handle = e->deref(handle)->next)
            e->deref(handle)->validate(Bucket::BlockHeader::Debug::sigFree);
    }

    for (auto header = _heap->_usedHead; header; header = header->next)
        header->validate(Bucket::BlockHeader::Debug::sigUsed);

    unlock();
}

String MemPool::printStats() const
{
    lock();

    ostringstream stream;
    stream.precision(1);
    stream.setf(std::ios::fixed);
    
    szt allocTotal = 0;
    szt usedSize = 0;
    szt usedCount = 0;
    szt freeCount = 0;
    for (auto& e : _buckets)
    {
        allocTotal += e->_chunkSizeTotal;
        usedSize += e->_usedSize;
        usedCount += e->_usedCount;
        freeCount += e->_freeCount;
    }

    allocTotal += _heap->_allocTotal;
    usedSize += _heap->_allocTotal;

    stream  << stringstream::indentInc << "{" << endl;

    stream  << "MemPool Id: " << _id << endl
            << "Total Allocated Bytes: " << allocTotal << endl
            << "Total Used Bytes: " << usedSize
                << " (" << Real(usedSize)/allocTotal*100 << "%)" << endl
            << "Block Header Size: " << sizeof(Bucket::BlockHeader) << endl
            << "Bucket Count: " << _bucketMap.size() << endl
            << "Bucket Blocks Used: " << usedCount << " / " << (freeCount+usedCount)
                << " (" << Real(usedCount)/(freeCount+usedCount)*100 << "%)" << endl;

    szt i = 0;
    for (auto& e : _buckets)
    {
        szt blockCount = e->_freeCount + e->_usedCount;
        stream  << "Bucket #" << i++ << ":" << endl
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

    stream  << "Heap:" << endl
            << stringstream::indentInc << "{" << endl
            << "Allocated Bytes: " << _heap->_allocTotal
                << " (" << Real(_heap->_allocTotal)/allocTotal*100 << "%)" << endl
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

    szt usedCount = 0;
    szt usedSize = 0;
    for (auto& e : _buckets)
    {
        usedCount += e->_usedCount;
        usedSize += e->_usedSize;
    }

    usedCount += _heap->_usedCount;
    usedSize += _heap->_allocTotal;

    stream  << "MemPool Id: " << _id << endl
            << "Total Used Bytes: " << usedSize << endl
            << "Total Blocks Used: " << usedCount << endl;

    szt bucket = 0;
    szt block = 0;
    for (auto& e : _buckets)
    {
        for (Bucket::Handle handle = e->_usedHead; handle; handle = e->deref(handle)->next, ++block)
        {
            auto header = e->deref(handle);
            header->validate(Bucket::BlockHeader::Debug::sigUsed);
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

    for (auto header = _heap->_usedHead; header; header = header->next, ++block)
    {
        header->validate(Bucket::BlockHeader::Debug::sigUsed);
        stream  << "Block #" << block << endl
                << "{" << endl
                << "    Allocator: Heap" << endl
                << "    Alloc Size: " << header->size
                    << " (" << Real(header->size)/usedSize*100 << "%)" << endl
                << "    Source File: " << c_str(header->debug.srcFile) << endl
                << "    Source Line: " << header->debug.srcLine << endl
                << "}" << endl;
    }

    unlock();

    return stream;
}

#endif //DEBUG

}
