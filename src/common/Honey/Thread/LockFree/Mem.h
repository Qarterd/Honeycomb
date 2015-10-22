// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Atomic.h"

namespace honey { namespace lockfree
{

/// Base node class, inherit from this class, add link members, and use as `Node` type in MemConfig 
struct MemNode
{
    MemNode()                               : ref(0), trace(false), del(false), recycleNext(nullptr) {}

    szt                     id;             ///< Unique id, used for index into "in-use mark" list in scan()
    int16                   threadId;       ///< Thread that created this node, used to return node to original free list

    Atomic<int>             ref;            ///< Reference count by all threads
    Atomic<bool>            trace;          ///< Used in scan()
    Atomic<bool>            del;            ///< Marked for deletion

    /// Thread-local node reference info. Each thread may contain a local reference to this node.
    struct Tlref
    {
        Tlref()                             : index(-1), ref(0) {}
        int8 index;                         ///< Index into thread-local ref list
        int8 ref;                           ///< Reference count by single thread
    };
    thread::Local<Tlref>    tlref;

    Atomic<MemNode*>        recycleNext;    ///< Next node in recycle bin
};

/// Base link class, contains a generic Cas-able data chunk.  The data chunk contains a pointer to a MemNode.
template<class Node>
struct MemLink
{
    /// Get node pointer
    Node* ptr() const                       { return reinterpret_cast<Node*>(*data); }

    Atomic<intptr_t> data;
};

/// Configuration interface for memory manager.  Inherit this class and override types and static members.
struct MemConfig
{
    typedef MemNode         Node;
    typedef MemLink<Node>   Link;

    /// Number of links per node
    static const int linkMax = 2;
    /// Number of links per node that may transiently point to a deleted node
    static const int linkDelMax = linkMax;
    /// Number of thread-local node references
    static const int8 tlrefMax = 6;

    typedef std::allocator<Node> Alloc;
    /// Get node allocator
    Alloc& getAlloc();

    /// Update all links in the node to point to active (non-deleted) nodes
    void cleanUpNode(Node& node);
    /// Remove all links to other nodes.  If concurrent is false then the faster storeRef can be used instead of casRef.
    void terminateNode(Node& node, bool concurrent);
};

/// Lock-free memory manager for concurrent algorithms.
/**
  * Based on the paper: "Efficient and Reliable Lock-Free Memory Reclamation Based on Reference Counting", Gidenstam, et al. - 2005
  *
  * This memory manager is more performant than the one described in the paper:
  * - Nodes to be freed are instead recycled, this avoids calls to the allocator, a performance bottleneck
  * - In scan(), a O(1) lookup table is used instead of a O(log n) set, this is possible because nodes are recycled
  *
  * The suggested way to reuse nodes is to simply add the freed node to a thread's private free list.
  * That is not a solution as a consumer thread which only frees will accumulate too many nodes.
  * When the O(1) private free list is too large, this memory manager falls back on a O(t) zero-contention recycling system
  * (where t is the number of threads).
  */
template<class Config>
class Mem : mt::NoCopy
{
public:
    typedef typename Config::Node Node;
    typedef typename Config::Link Link;

private:
    /// Per thread data.  Linked list is maintained of all threads using the memory manager.
    struct ThreadData
    {
        ThreadData(Mem& mem, int id) :
            mem(mem),
            id(id),
            nodeCount(0),
            delNodes(new DelNode[mem._threshClean]),
            delHead(nullptr),
            delCount(0),
            recycleBins(new Recycle[mem._threadMax])
        {
            tlrefs.fill(nullptr);
            for (szt i = 0; i < tlrefs.size(); ++i)
                tlrefFreeList.push_back(i);

            for (int i = 0; i < mem._threshClean; ++i)
                delNodeFreeList.push_back(&delNodes[i]);
        }

        ~ThreadData()
        {
            //Add to free list all nodes waiting to be reclaimed 
            for (DelNode* delNode = delHead; delNode; delNode = delNode->next)
                nodeFreeList.push_back(delNode->node);
            //Add to free list all recycled nodes
            for (int ti = 0; ti < mem._threadDataCount; ++ti)
                for (Node* node = recycleBins[ti].head; node; node = static_cast<Node*>(node->recycleNext.load()))
                    nodeFreeList.push_back(node);
            //Delete nodes in free list
            deleteRange(nodeFreeList, mem._config.getAlloc());

            deleteArray(delNodes);
            deleteArray(recycleBins);
        }

        Mem&                mem;
        int                 id;

        vector<Node*>       nodeFreeList;
        szt                 nodeCount;

        array<Atomic<Node*>, Config::tlrefMax> tlrefs;
        vector<int8>        tlrefFreeList;

        struct DelNode
        {
            DelNode()                       : node(nullptr), claim(0), done(false), next(nullptr) {}

            Atomic<Node*>   node;
            Atomic<int>     claim;
            Atomic<bool>    done;
            DelNode*        next;
        };
        DelNode*            delNodes;
        vector<DelNode*>    delNodeFreeList;
        vector<bool>        delTlrefs;
        DelNode*            delHead;
        szt                 delCount;

        /// Lock-free list of recycled free nodes
        /**
          * One bin is maintained for each other thread, so at most there are two threads accessing a bin (this + other).
          * The producer thread (other) adds to the tail and the consumer thread (this) removes from the head.
          * In this way, contention for the recycle bin is completely eliminated.
          */
        struct Recycle
        {
            Recycle()                       : head(nullptr), tail(nullptr) {}
            Atomic<Node*>   head;
            Atomic<Node*>   tail;
        };
        Recycle*            recycleBins;
    };
    
public:
    /**
      * \param config
      * \param threadMax    Max number of threads that can access the memory manager.  Use a thread pool so the threads have a longer life cycle than the mem manager.
      */ 
    Mem(Config& config, int threadMax = 8) :
        _config(config),
        _threadMax(threadMax),
        _threshClean(_threadMax*(Config::tlrefMax + Config::linkMax + Config::linkDelMax + 1)),
        _threshScan(Config::tlrefMax*2 < _threshClean ? Config::tlrefMax*2 : _threshClean),
        _threadDataList(new ThreadData*[_threadMax]),
        _threadDataCount(0),
        _threadData(bind(&Mem::initThreadData, this)),
        _nodeId(0)
    {
        std::fill(_threadDataList, _threadDataList + _threadMax, nullptr);
    }

    ~Mem()
    {
        //Delete all thread data
        for (int ti = 0; ti < _threadDataCount; ++ti)
            delete_(_threadDataList[ti]);
        delete_(_threadDataList);
    }

    Node& createNode()
    {
        ThreadData& td = threadData();

        if (td.nodeFreeList.size() == 0)
        {
            recycleScan(td);
            //If no recycled nodes were found, allocate more nodes 
            if (td.nodeFreeList.size() == 0)
            {
                szt nextCount = td.nodeCount*2 + 1;
                szt newCount = nextCount - td.nodeCount;
                for (szt i = 0; i < newCount; ++i)
                {
                    td.nodeFreeList.push_back(new (_config.getAlloc().allocate(1)) Node);
                    td.nodeFreeList.back()->id = _nodeId++;
                    td.nodeFreeList.back()->threadId = td.id;
                }
                td.nodeCount = nextCount;
            }
        }

        //Get free node
        Node* node = td.nodeFreeList.back();
        td.nodeFreeList.pop_back();
        ref(*node);
        return *node;
    }

    void deleteNode(Node& node)
    {
        ThreadData& td = threadData();

        node.del = true;
        node.trace = false;

        //Get free del node
        assert(td.delNodeFreeList.size() > 0, "Not enough del nodes, algorithm problem");
        auto& delNode = *td.delNodeFreeList.back();
        td.delNodeFreeList.pop_back();
        //Init del node tlref lookup
        if (node.id >= td.delTlrefs.size())
            td.delTlrefs.resize(node.id*2+1);
        td.delTlrefs[node.id] = false;

        delNode.done = false;
        delNode.node = &node;
        delNode.next = td.delHead;
        td.delHead = &delNode; ++td.delCount;
        while(true)
        {
            if (td.delCount == _threshClean) cleanUpLocal();
            if (td.delCount >= _threshScan) scan();
            if (td.delCount == _threshClean) cleanUpAll();
            else break;
        }
    }

    /// Dereference a link, protects with tlref. May return null.
    Node* deRefLink(Link& link)
    {
        ThreadData& td = threadData();
        //Get free tlref index
        assert(td.tlrefFreeList.size() > 0, "Not enough thread-local node references");
        int8 index = td.tlrefFreeList.back();

        Node* node = nullptr;
        while(true)
        {
            node = link.ptr();
            //Set up tlref
            td.tlrefs[index] = node;
            //Ensure that link is protected
            if (link.ptr() == node) break;
        }

        //Only add tlref if pointer is valid
        if (node)
        {
            auto& tlref = *node->tlref;
            //If tlref is already referenced by this thread then we don't need a new tlref
            if (tlref.ref++ > 0)
                td.tlrefs[index] = nullptr;
            else
            {
                tlref.index = index;
                td.tlrefFreeList.pop_back();
            }
        }
        return node;
    }

    /// Add reference to node, sets up tlref
    void ref(Node& node)
    {
        auto& tlref = *node.tlref;
        //If tlref is already referenced by this thread then we don't need a new tlref
        if (tlref.ref++ > 0) return;

        ThreadData& td = threadData();
        //Get free tlref index
        assert(td.tlrefFreeList.size() > 0, "Not enough thread-local node references");
        int8 index = td.tlrefFreeList.back();
        td.tlrefFreeList.pop_back();
        //Set up tlref
        tlref.index = index;
        td.tlrefs[index] = &node;
    }

    /// Release a reference to a node, clears tlref
    void releaseRef(Node& node)
    {
        auto& tlref = *node.tlref;
        //Only release if this thread has no more references
        if (--tlref.ref > 0) return;
        assert(tlref.ref == 0, "Thread-local node reference already released");

        ThreadData& td = threadData();
        //Return tlref index to free list
        td.tlrefs[tlref.index] = nullptr;
        td.tlrefFreeList.push_back(tlref.index);
    }

    /// Compare and swap link.  Set link in a concurrent environment.  Returns false if the link was changed by another thread.
    bool casRef(Link& link, const Link& val, const Link& old)
    {
        if (link.data.cas(val.data, old.data))
        {
            if (val.ptr())
            {
                ++val.ptr()->ref;
                val.ptr()->trace = false;
            }
            if (old.ptr()) --old.ptr()->ref;
            return true;
        }
        return false;
    }

    /// Set link in a single-threaded environment
    void storeRef(Link& link, const Link& val)
    {
        Link old = link;
        link = val;
        if (val.ptr())
        {
            ++val.ptr()->ref;
            val.ptr()->trace = false;
        }
        if (old.ptr()) --old.ptr()->ref;
    }

private:
    /// Thread data ptr holder.  This indirection gives us control of the thread data life cycle, otherwise it would be deleted on thread exit.
    struct ThreadDataPtr
    {
        ThreadDataPtr(ThreadData* ptr)      : ptr(ptr) {}
        ThreadData* ptr;
    };

    ThreadDataPtr* initThreadData()
    {
        SpinLock::Scoped _(_threadDataLock);
        //Increase thread count
        assert(_threadDataCount < _threadMax, "Too many threads accessing memory manager");
        //Create new data and add to list
        ThreadData* threadData = new ThreadData(*this, _threadDataCount);
        _threadDataList[_threadDataCount] = threadData;
        _threadDataCount++;
        return new ThreadDataPtr(threadData);
    }

    ThreadData& threadData()                { return *_threadData->ptr; }

    /// Update nodes deleted by this thread so links referencing deleted nodes are replaced with live nodes 
    void cleanUpLocal()
    {
        ThreadData& td = threadData();
        for (auto* delNode = td.delHead; delNode; delNode = delNode->next)
            _config.cleanUpNode(*delNode->node);
    }

    /// Update nodes deleted by all threads so links referencing deleted nodes are replaced with live nodes 
    void cleanUpAll()
    {
        for (int ti = 0; ti < _threadDataCount; ++ti)
        {
            ThreadData* td = _threadDataList[ti];
            for (int i = 0; i < _threshClean; ++i)
            {
                auto& delNode = td->delNodes[i];
                Node* node = delNode.node;
                if (node && !delNode.done)
                {
                    ++delNode.claim;
                    if (node == delNode.node)
                        _config.cleanUpNode(*node);
                    --delNode.claim;
                }
            }
        }
    }

    /// Searches through deleted nodes and attempts to reclaim them.  Nodes pointed to by tlrefs can't be reclaimed.
    void scan()
    {
        ThreadData& td = threadData();

        //Set trace to make sure ref == 0 is consistent across tlref check below
        for (auto* delNode = td.delHead; delNode; delNode = delNode->next)
        {
            Node& node = *delNode->node;
            if (node.ref == 0)
            {
                node.trace = true;
                if (node.ref != 0)
                    node.trace = false;
            }
        }

        //Flag all del nodes that have a tlref so they are not reclaimed
        for (int ti = 0; ti < _threadDataCount; ++ti)
        {
            ThreadData* tdata = _threadDataList[ti];
            for (szt i = 0; i < tdata->tlrefs.size(); ++i)
            {
                Node* node = tdata->tlrefs[i];
                if (node && node->id < td.delTlrefs.size())
                    td.delTlrefs[node->id] = true;
            }
        }

        //Reclaim nodes and build new list of del nodes that could not be reclaimed
        typename ThreadData::DelNode* newDelHead = nullptr;
        szt newDelCount = 0;

        while (td.delHead)
        {
            auto* delNode = td.delHead;
            td.delHead = delNode->next;
            Node& node = *delNode->node;
            if (node.ref == 0 && node.trace && !td.delTlrefs[node.id])
            {
                delNode->node = nullptr;
                if (delNode->claim == 0)
                {
                    _config.terminateNode(node, false);
                    //Free the node
                    td.delNodeFreeList.push_back(delNode);
                    td.nodeFreeList.push_back(&node);
                    recycleFree(td);
                    continue;
                }
                _config.terminateNode(node, true);
                delNode->done = true;
                delNode->node = &node;
            }
            td.delTlrefs[node.id] = false;
            delNode->next = newDelHead;
            newDelHead = delNode;
            ++newDelCount;
        }

        td.delHead = newDelHead;
        td.delCount = newDelCount;
    }

    /// Move nodes from this thread's private free list to their respective owners recycle bins
    void recycleFree(ThreadData& td)
    {
        // Only recycle if private free list is too large
        if (td.nodeFreeList.size() <= td.nodeCount*2) return;
        // Return a chunk of nodes (up to _threshClean amount). Returning in chunks reduces number of recycle scans.
        sdt newSize = td.nodeCount*2 - _threshClean;
        if (newSize < td.nodeCount) newSize = td.nodeCount;
        while (td.nodeFreeList.size() > newSize)
        {
            Node& node = *td.nodeFreeList.back();
            td.nodeFreeList.pop_back();
            auto& rec = _threadDataList[node.threadId]->recycleBins[td.id];
            node.recycleNext = nullptr;
            if (rec.tail) rec.tail->recycleNext = &node;    //Previous tail can be consumed immediately atfer recycle next is set
            rec.tail = &node;
            if (!rec.head) rec.head = &node;                //Head is null only on first recycle
        }
    }

    /// Loops through this thread's recycle bins (one for each other thread) and reclaims nodes
    void recycleScan(ThreadData& td)
    {
        // Take only up to _threshClean recycled nodes so loop doesn't take too long
        sdt newSize = td.nodeCount < _threshClean ? td.nodeCount : _threshClean;
        for (int ti = 0; ti < _threadDataCount && td.nodeFreeList.size() < newSize; ++ti)
        {
            auto& rec = td.recycleBins[ti];
            Node* node = rec.head;
            if (!node) continue;
            Node* next = static_cast<Node*>(node->recycleNext.load());
            if (!next) continue;            //If next is null then this is the tail, not allowed to consume tail
            do
            {
                td.nodeFreeList.push_back(node);
                node = next;
                next = static_cast<Node*>(node->recycleNext.load());
            } while (next && td.nodeFreeList.size() < newSize);
            rec.head = node;                //Update head to next unconsumed node
        }
    }

    Config&                         _config;
    const int                       _threadMax;
    const int                       _threshClean;
    const int                       _threshScan;
    ThreadData**                    _threadDataList;
    Atomic<int>                     _threadDataCount;
    thread::Local<ThreadDataPtr>    _threadData;
    SpinLock                        _threadDataLock;
    Atomic<szt>                     _nodeId;
};


} }
