// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/Thread.h"
#include "Honey/Thread/Atomic.h"
#include "Honey/Memory/SmallAllocator.h"

namespace honey { namespace lockfree
{

/// Base node class, inherit from this class, add link members, and use as `Node` type in HazardMemConfig 
struct HazardMemNode
{
    HazardMemNode()                         : ref(0), trace(false), del(false) {}

    Atomic<int>     ref;        ///< Reference count by all threads
    Atomic<bool>    trace;      ///< Used in scan()
    Atomic<bool>    del;        ///< Marked for deletion
};

/// Base link class, contains a generic Cas-able data chunk.  The data chunk contains a pointer to a HazardMemNode.
template<class Node>
struct HazardMemLink
{
    /// Get node pointer
    Node* ptr() const                       { return reinterpret_cast<Node*>(*data); }

    Atomic<intptr_t> data;
};

/// Configuration interface for memory manager.  Inherit this class and override types and static members.
struct HazardMemConfig
{
    typedef HazardMemNode           Node;
    typedef HazardMemLink<Node>     Link;
    /// Allocator for nodes, should be lock-free
    typedef SmallAllocator<Node>    Alloc;
    
    /// Number of links per node
    static const uint8 linkMax = 2;
    /// Number of links per node that may transiently point to a deleted node
    static const uint8 linkDelMax = linkMax;
    /// Number of thread-local hazard pointers
    static const uint8 hazardMax = 6;

    /// Update all links in the node to point to active (non-deleted) nodes
    void cleanUpNode(Node& node);
    /// Remove all links to other nodes.  If concurrent is false then the faster storeRef can be used instead of casRef.
    void terminateNode(Node& node, bool concurrent);
};

/// Lock-free memory manager, provides safe memory reclamation for concurrent algorithms.
/**
  * Based on the paper: "Efficient and Reliable Lock-Free Memory Reclamation Based on Reference Counting", Gidenstam, et al. - 2005
  */
template<class Config>
class HazardMem : mt::NoCopy
{
public:
    typedef typename Config::Node Node;
    typedef typename Config::Link Link;
    typedef typename Config::Alloc::template rebind<Node>::other Alloc;
    
private:
    /// Per thread data.  Linked list is maintained of all threads using the memory manager.
    struct ThreadData
    {
        ThreadData(HazardMem& mem) :
            mem(mem),
            delNodes(new DelNode[mem._threshClean]),
            delHazards(mem._alloc),
            delHead(nullptr),
            delCount(0)
        {
            hazards.fill(nullptr);
            hazardRefCounts.fill(0);
            
            hazardFreeList.reserve(hazards.size());
            for (auto i : range(hazards.size())) hazardFreeList.push_back(i);
            
            delNodeFreeList.reserve(mem._threshClean);
            for (auto i : range(mem._threshClean)) delNodeFreeList.push_back(&delNodes[i]);
        }

        ~ThreadData()
        {
            //Free all nodes waiting to be reclaimed
            for (DelNode* delNode = delHead; delNode; delNode = delNode->next)
            {
                mem._alloc.destroy(delNode->node.load());
                mem._alloc.deallocate(delNode->node, 1);
            }
        }
        
        struct DelNode
        {
            DelNode()                       : node(nullptr), claim(0), done(false), next(nullptr) {}

            Atomic<Node*>       node;
            Atomic<int>         claim;
            Atomic<bool>        done;
            DelNode*            next;
        };
        typedef set<Node*, std::less<Node*>, typename Alloc::template rebind<Node*>::other> NodeLookup;
        
        HazardMem&              mem;
        array<Atomic<Node*>, Config::hazardMax> hazards;
        array<uint8, Config::hazardMax> hazardRefCounts; ///< Reference count by single thread
        vector<uint8>           hazardFreeList;
        UniquePtr<DelNode[]>    delNodes;
        vector<DelNode*>        delNodeFreeList;
        NodeLookup              delHazards;
        DelNode*                delHead;
        szt                     delCount;
    };
    
public:
    /**
      * \param threadMax    Max number of threads that can access the memory manager.
      *                     Use a thread pool and ensure that it has a longer life cycle than the mem manager.
      */ 
    HazardMem(Config& config, const Alloc& alloc, int threadMax) :
        _config(config),
        _alloc(alloc),
        _threadMax(threadMax),
        _threshClean(_threadMax*(Config::hazardMax + Config::linkMax + Config::linkDelMax + 1)),
        _threshScan(Config::hazardMax*2 < _threshClean ? Config::hazardMax*2 : _threshClean),
        _threadDataList(_threadMax),
        _threadDataCount(0),
        _threadData(bind(&HazardMem::initThreadData, this)) {}
    
    template<class... Args>
    Node& createNode(Args&&... args)
    {
        Node* node = _alloc.allocate(1);
        _alloc.construct(node, forward<Args>(args)...);
        ref(*node);
        return *node;
    }

    void deleteNode(Node& node)
    {
        ThreadData& td = threadData();

        node.del = true;
        node.trace = false;

        //get free del node
        assert(td.delNodeFreeList.size() > 0, "Not enough del nodes, algorithm problem");
        auto& delNode = *td.delNodeFreeList.back();
        td.delNodeFreeList.pop_back();

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

    /// Dereference a link, protects with hazard pointer. May return null.
    Node* deRefLink(Link& link)
    {
        ThreadData& td = threadData();
        //get free hazard index
        assert(td.hazardFreeList.size() > 0, "Not enough hazard pointers");
        uint8 index = td.hazardFreeList.back();

        Node* node = nullptr;
        while(true)
        {
            node = link.ptr();
            //init new hazard
            td.hazards[index] = node;
            //ensure that link is protected
            if (link.ptr() == node) break;
        }

        //only add hazard if pointer is valid
        if (node)
        {
            //check if hazard is already referenced by this thread
            sdt found = -1;
            for (auto i: range(td.hazards.size())) if (i != index && td.hazards[i] == node) { found = i; break; }
            if (found >= 0)
            {
                //don't need a new hazard
                td.hazards[index] = nullptr;
                index = found;
            }
            else
                //use new hazard
                td.hazardFreeList.pop_back();
            ++td.hazardRefCounts[index];
        }
        return node;
    }

    /// Add reference to node, sets up hazard pointer
    void ref(Node& node)
    {
        ThreadData& td = threadData();
        //check if hazard is already referenced by this thread
        //we could use a map here but the hazard list is tiny for known algorithms
        sdt index = -1;
        for (auto i: range(td.hazards.size())) if (td.hazards[i] == &node) { index = i; break; }
        
        if (index < 0)
        {
            //get free hazard index
            assert(td.hazardFreeList.size() > 0, "Not enough hazard pointers");
            index = td.hazardFreeList.back();
            td.hazardFreeList.pop_back();
            //init new hazard
            td.hazards[index] = &node;
        }
        ++td.hazardRefCounts[index];
    }

    /// Release a reference to a node, clears hazard pointer
    void releaseRef(Node& node)
    {
        ThreadData& td = threadData();
        //get associated hazard pointer
        sdt index = -1;
        for (auto i: range(td.hazards.size())) if (td.hazards[i] == &node) { index = i; break; }
        assert(index >= 0, "Hazard pointer not found");
        
        //only release if this thread has no more references
        assert(td.hazardRefCounts[index], "Hazard pointer already released");
        if (--td.hazardRefCounts[index]) return;
        //release hazard index to free list
        td.hazards[index] = nullptr;
        td.hazardFreeList.push_back(index);
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
    /// This indirection gives us control of the thread data life cycle, otherwise it would be deleted on thread exit.
    struct ThreadDataRef { ThreadData& obj; };

    ThreadDataRef* initThreadData()
    {
        SpinLock::Scoped _(_threadDataLock);
        //increase thread count
        assert(_threadDataCount < _threadMax, "Too many threads accessing memory manager");
        //create new data and add to list
        auto threadData = new ThreadData(*this);
        _threadDataList[_threadDataCount] = UniquePtr<ThreadData>(threadData);
        ++_threadDataCount; //must increment only after initing element, or concurrent ops will fail
        return new ThreadDataRef{*threadData};
    }

    ThreadData& threadData()                { return _threadData->obj; }

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
        for (auto ti : range(_threadDataCount.load()))
        {
            ThreadData* td = _threadDataList[ti];
            for (auto i : range(_threshClean))
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

    /// Searches through deleted nodes and attempts to reclaim them.  Nodes pointed to by hazards can't be reclaimed.
    void scan()
    {
        ThreadData& td = threadData();

        //set trace to make sure ref == 0 is consistent across hazard check below
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

        //flag all del nodes that have a hazard so they are not reclaimed
        for (auto ti : range(_threadDataCount.load()))
        {
            ThreadData* tdata = _threadDataList[ti];
            for (auto i : range(tdata->hazards.size()))
            {
                Node* node = tdata->hazards[i];
                if (node) td.delHazards.insert(node);
            }
        }

        //reclaim nodes and build new list of del nodes that could not be reclaimed
        typename ThreadData::DelNode* newDelHead = nullptr;
        szt newDelCount = 0;

        while (td.delHead)
        {
            auto* delNode = td.delHead;
            td.delHead = delNode->next;
            Node& node = *delNode->node;
            if (node.ref == 0 && node.trace && !td.delHazards.count(&node))
            {
                delNode->node = nullptr;
                if (delNode->claim == 0)
                {
                    _config.terminateNode(node, false);
                    //Free the node
                    td.delNodeFreeList.push_back(delNode);
                    _alloc.destroy(&node);
                    _alloc.deallocate(&node, 1);
                    continue;
                }
                _config.terminateNode(node, true);
                delNode->done = true;
                delNode->node = &node;
            }
            delNode->next = newDelHead;
            newDelHead = delNode;
            ++newDelCount;
        }
        
        td.delHazards.clear();
        td.delHead = newDelHead;
        td.delCount = newDelCount;
    }

    Config&                         _config;
    Alloc                           _alloc;
    const int                       _threadMax;
    const szt                       _threshClean;
    const szt                       _threshScan;
    vector<UniquePtr<ThreadData>>   _threadDataList;
    Atomic<int>                     _threadDataCount;
    thread::Local<ThreadDataRef>    _threadData;
    SpinLock                        _threadDataLock;
};


} }
