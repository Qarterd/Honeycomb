// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Enum.h"
#include "Honey/Misc/MtMap.h"
#include "Honey/Memory/SmallAllocator.h"
#include "Honey/Memory/SharedPtr.h"

namespace honey
{

/// Dependency node for insertion into graph
template<class Data_, class Key_ = Id, template<class> class Alloc_ = SmallAllocator>
class DepNode
{
public:
    enum class DepType
    {
        out,    ///< this node depends on the target node
        in      ///< this node is depended on by the target node
    };
    static const int depTypeMax = 2;
    
    typedef Data_ Data;
    typedef Key_ Key;
    template<class T> using Alloc = Alloc_<T>;
    typedef stdutil::unordered_map<Key, DepType, Alloc> DepMap;
    
    DepNode(const Data& data = Data(), const Key& key = Key())      : _data(data), _key(key) {}

    /// Set the data that this node contains
    void setData(const Data& data)              { _data = data; }
    const Data& getData() const                 { return _data; }
    Data& getData()                             { return _data; }
    /// Implicit cast to the data at this node
    operator const Data&() const                { return getData(); }
    operator Data&()                            { return getData(); }
    /// Get the data at this node
    const Data& operator*() const               { return getData(); }
    Data& operator*()                           { return getData(); }
    const Data& operator->() const              { return getData(); }
    Data& operator->()                          { return getData(); }

    /// Set the key used to identify this node
    void setKey(const Key& key)                 { _key = key; }
    const Key& getKey() const                   { return _key; }
    Key& getKey()                               { return _key; }

    /// Add a dependency link
    void add(const Key& key, DepType type = DepType::out)
    {
        //Can't depend on itself
        if (key == _key) return;
        _deps[key] = type;
    }

    /// Remove a dependency link
    void remove(const Key& key)                 { _deps.erase(key); }
    /// Remove all dependency links
    void clear()                                { _deps.clear(); }

    /// Get dep links
    const DepMap& deps() const                  { return _deps; }

    /// Get opposite dependency type
    static DepType depTypeOpp(DepType type)     { return type == DepType::out ? DepType::in : DepType::out; }

private:
    Data    _data;
    Key     _key;
    DepMap  _deps;
};



/// Dependency graph.  Collects nodes and builds a searchable directed graph.
/**
  * Nodes can be added and removed from the graph freely, even after calls to condense().
  * Do not change a node's dependency list while it is still in the graph, the node must be removed first and re-added after.
  */
template<class DepNode_>
class DepGraph
{
    typedef DepNode_ DepNode;
    typedef typename DepNode::Data Data;
    typedef typename DepNode::Key Key;
    typedef typename DepNode::DepType DepType;
    template<class T> using Alloc = typename DepNode::template Alloc<T>;
    
public:
    class Vertex;

private:
    typedef stdutil::unordered_map<Key, Vertex*, Alloc> VertexMap;
    typedef list<Vertex, Alloc<Vertex>> VertexList;

public:
    typedef stdutil::unordered_set<DepNode*, Alloc>        NodeList;
    typedef stdutil::unordered_set<const DepNode*, Alloc>  NodeListConst;
    typedef stdutil::unordered_set<Key, Alloc>             KeyList;

    /// A vertex is initially associated with one key and acts like a multi-map, storing all nodes and graph links of DepNodes matching that key.
    /**
      * After condense() is called cyclical subgraphs will be merged into one vertex.
      * The merged vertex thus becomes associated with all keys in the subgraph.
      */
    class Vertex
    {
        friend class DepGraph;
    public:
        typedef stdutil::unordered_map<Vertex*, int, Alloc> LinkMap;
        typedef stdutil::unordered_map<const Vertex*, int, Alloc> LinkMapConst;

        /// Get all nodes that constitute this vertex
        const NodeList& nodes()                         { return nodeList; }
        const NodeListConst& nodes() const              { return reinterpret_cast<const NodeListConst&>(nodeList); }
        /// Get all keys mapped to this vertex
        const KeyList& keys() const                     { return keyList; }
        /// Get all vertices along in/out links
        auto links(DepType type = DepType::out)         { return honey::keys(const_cast<const Vertex*>(this)->linkMaps[static_cast<int>(type)]); }
        auto links(DepType type = DepType::out) const   { return honey::keys(reinterpret_cast<const LinkMapConst&>(linkMaps[static_cast<int>(type)])); }

    private:
        /// Add link, increment reference by count
        void add(DepType type, Vertex* vertex, int count = 1)
        {
            auto& linkMap = this->linkMap(type);
            auto itMap = linkMap.find(vertex);
            auto& refCount = (itMap == linkMap.end()) ? linkMap[vertex] = 0 : itMap->second;
            refCount += count;
        }

        /// Remove link, decrement reference by count
        void remove(DepType type, Vertex* vertex, int count = 1)
        {
            auto& linkMap = this->linkMap(type);
            auto itMap = linkMap.find(vertex);
            assert(itMap != linkMap.end(), sout() <<    "Unable to remove dependency link. Vertex: " << *keyList.begin() <<
                                                        " ; Link Type: " << static_cast<int>(type) << " ; Link Key: " << *vertex->keyList.begin());
            auto& refCount = itMap->second;
            refCount -= count;
            if (refCount <= 0) linkMap.erase(itMap);
        }

        const LinkMap& linkMap(DepType type) const      { return linkMaps[static_cast<int>(type)]; }
        LinkMap& linkMap(DepType type)                  { return linkMaps[static_cast<int>(type)]; }

        /// A phantom vertex exists only because it was referenced as a dependency, but it is otherwise uninitialized and not considered for graph operations
        bool isPhantom() const                          { return nodeList.empty(); }
        /// A normal vertex is associated with one key, a merged vertex after condensation is associated with multiple keys.
        bool isMerged() const                           { return keyList.size() > 1; }

        typename VertexList::iterator itAlloc;
        NodeList nodeList;
        KeyList keyList;
        array<LinkMap, DepNode::depTypeMax> linkMaps;
    };

private:
    typedef vector<Vertex*, Alloc<Vertex*>> VertexPList;
    typedef stdutil::unordered_set<Vertex*, Alloc> VertexSet;

    struct CondenseData
    {
        CondenseData() : preOrd(0) {}

        typedef stdutil::unordered_map<Vertex*, int, Alloc> PreOrdMap;
        //Merge vertex -> Component vertices
        typedef stdutil::unordered_map<Vertex*, VertexSet, Alloc> MergeMap;
        //Component vertices -> Merge vertex
        typedef stdutil::unordered_map<Vertex*, Vertex*, Alloc> MergeMapR;

        VertexPList stackS;
        VertexPList stackP;
        int preOrd;
        PreOrdMap preOrdMap;
        VertexSet assignedList;
        MergeMap mergeMap;
        MergeMapR mergeMapR;
    };

    mtkey(k_vertex)
    mtkey(k_linkMap)
    mtkey(k_linkIt)

    typedef MtMap<  Vertex*, k_vertex,
                    const typename Vertex::LinkMap*, k_linkMap,
                    typename Vertex::LinkMap::const_iterator, k_linkIt
                    > VertexLink;

    typedef vector<VertexLink, Alloc<VertexLink>> VertexLinkList;

public:
    /// Add a node to the graph
    bool add(DepNode& node)
    {
        auto& vertex = createVertex(node.getKey());
        //Check if already added
        if (!vertex.nodeList.insert(&node).second) return false;

        for (auto& e : node.deps())
        {
            auto& key = e.first;
            auto type = e.second;

            //Create phantom vertex if it doesn't exist
            auto& depVertex = createVertex(key);
            //Vertex can't depend on itself
            if (&depVertex == &vertex) continue;

            switch (type)
            {
            case DepType::out:
                vertex.add(DepType::out, &depVertex);
                depVertex.add(DepType::in, &vertex);
                break;
            case DepType::in:
                vertex.add(DepType::in, &depVertex);
                depVertex.add(DepType::out, &vertex);
                break;
            }
        }
        return true;
    }

    /// Remove a node from the graph
    bool remove(DepNode& node)
    {
        auto itMap = _vertexMap.find(node.getKey());
        if (itMap == _vertexMap.end()) return false;

        auto& vertex = *itMap->second;

        //Node must be referenced
        auto itNode = vertex.nodeList.find(&node);
        if (itNode == vertex.nodeList.end()) return false;
        vertex.nodeList.erase(itNode);

        for (auto& e : node.deps())
        {
            auto& key = e.first;
            auto type = e.second;

            //Find dependency vertex in map
            auto itMap = _vertexMap.find(key);
            assert(itMap != _vertexMap.end(), sout() << "Unable to remove dependency. Node: " << node.getKey() <<
                                                        " ; DepType: " << static_cast<int>(type) << " ; DepKey: " << key);
            auto& depVertex = *itMap->second;
            //Vertex can't depend on itself
            if (&depVertex == &vertex) continue;

            switch (type)
            {
            case DepType::out:
                vertex.remove(DepType::out, &depVertex);
                depVertex.remove(DepType::in, &vertex);
                break;
            case DepType::in:
                vertex.remove(DepType::in, &depVertex);
                depVertex.remove(DepType::out, &vertex);
                break;
            }

            //Don't keep around unreferenced phantoms
            if (depVertex.isPhantom() && depVertex.linkMap(DepType::in).empty() && depVertex.linkMap(DepType::out).empty())
                deleteVertex(depVertex);
        }

        //Don't keep around unreferenced phantoms
        if (vertex.isPhantom() && vertex.linkMap(DepType::in).empty() && vertex.linkMap(DepType::out).empty())
            deleteVertex(vertex);
        //If we changed a merged vertex then we've probably broken the cyclic dependency so we should decompose it
        else if (vertex.isMerged())
            decondense(vertex);
        return true;
    }

    /// Clear graph of all nodes
    void clear()
    {
        _vertexList.clear();
        _vertexMap.clear();
    }

    /// Depth-first pre-order iterator over vertices.
    /**
      * Each iteration step visits a vertex which has a list of contained nodes. \see Vertex
      *
      * The first iteration step returns the start vertex, then it moves along the dependency graph edges according to the
      * DepType: `out` (depends on), or `in` (depended on by).
      */
    template<class Vertex_>
    class Iter_
    {
        friend class DepGraph;
        template<class Vertex> friend class NodeIter_;

    public:
        typedef std::forward_iterator_tag   iterator_category;
        typedef Vertex_                     value_type;
        typedef sdt                         difference_type;
        typedef Vertex_*                    pointer;
        typedef Vertex_&                    reference;
        
        Iter_() :
            _graph(nullptr), _vertex(nullptr), _type(DepType::out), _skipEdges(false) {}

        Iter_(const DepGraph& graph, optional<const Key&> start, DepType type) :
            _graph(&graph) { reset(start, type); }

        Iter_& operator++()
        {
            if (!_vertex) return *this;

            if (_skipEdges)
            {
                while (!_stack.empty() && _stack.back()[k_vertex()] == _vertex) _stack.pop_back();
                _skipEdges = false;
            }

            while (_vertexIt != _graph->_vertexList.end() || !_stack.empty())
            {
                if (_stack.empty())
                {
                    //move to next root vertex in global list
                    ++_vertexIt;
                    nextRoot();
                }
                else
                {
                    //move along links
                    if (_stack.back()[k_linkIt()] == _stack.back()[k_linkMap()]->end()) { _stack.pop_back(); continue; }
                    _vertex = (_stack.back()[k_linkIt()]++)->first;
                }
                if (!_vertex || _vertex->isPhantom() || !_visited.insert(_vertex).second) continue;

                //next iter move along links
                _stack.push_back(mtmap( k_vertex() = _vertex,
                                        k_linkMap() = &_vertex->linkMap(_type),
                                        k_linkIt() = _vertex->linkMap(_type).begin() ));
                return *this;
            }

            _vertex = nullptr;
            return *this;
        }

        Iter_ operator++(int)                               { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const Iter_& rhs) const             { return _vertex == rhs._vertex; }
        bool operator!=(const Iter_& rhs) const             { return !operator==(rhs); }

        reference operator*() const                         { assert(_vertex); return *_vertex; }
        pointer operator->() const                          { return &operator*(); }

        /// Reset iterator to begin at vertex in graph
        void reset(optional<const Key&> start = optnull, DepType type = DepType::out)
        {
            _type = type;
            _skipEdges = false;
            _vertexIt = _graph->_vertexList.end();
            _stack.clear();
            _visited.clear();

            if (start)
            {
                //find start vertex by key (ignore phantom)
                auto itMap = _graph->_vertexMap.find(*start);
                _vertex = itMap != _graph->_vertexMap.end() && !itMap->second->isPhantom() ? itMap->second : nullptr;
            }
            else
            {
                //get first root vertex in global list (ignore phantom)
                _vertexIt = _graph->_vertexList.begin();
                nextRoot();
            }
            if (!_vertex) return;

            //next iter move along links
            _visited.insert(_vertex);
            _stack.push_back(mtmap( k_vertex() = _vertex,
                                    k_linkMap() = &_vertex->linkMap(_type),
                                    k_linkIt() = _vertex->linkMap(_type).begin() ));
        }

        /// Skip the current vertex's edges on next step of this iterator
        void skipEdges()                                    { _skipEdges = true; }
        
    private:
        void nextRoot()
        {
            auto typeOpp = DepNode::depTypeOpp(_type);
            for (; _vertexIt != _graph->_vertexList.end() && (_vertexIt->isPhantom() || !_vertexIt->linkMap(typeOpp).empty()); ++_vertexIt);
            _vertex = _vertexIt != _graph->_vertexList.end() ? const_cast<Vertex*>(&*_vertexIt) : nullptr;
        }
        
        const DepGraph*     _graph;
        Vertex*             _vertex;
        DepType             _type;
        bool                _skipEdges;
        typename VertexList::const_iterator _vertexIt;
        VertexLinkList      _stack;
        VertexSet           _visited;
    };
    typedef Iter_<Vertex> Iter;
    typedef Iter_<const Vertex> ConstIter;

    /// Get a depth-first pre-order range starting from a vertex.  If start vertex is null then range is over all vertices, starting from all roots.
    Range_<ConstIter, ConstIter> range(optional<const Key&> start = optnull, DepType type = DepType::out) const     { return honey::range(ConstIter(*this, start, type), ConstIter()); }
    Range_<Iter, Iter>           range(optional<const Key&> start = optnull, DepType type = DepType::out)           { return honey::range(Iter(*this, start, type), Iter()); }


    /// Depth-first pre-order iterator over nodes.  Iterates over nodes that constitute each vertex in a vertex range.
    template<class Vertex_>
    class NodeIter_
    {
    public:
        typedef Iter_<Vertex_>                  Iter;
        typedef typename std::conditional<std::is_const<Vertex_>::value, NodeListConst, NodeList>::type  NodeList;
        typedef typename mt::removePtr<typename NodeList::value_type>::type     DepNode;
        typedef std::forward_iterator_tag       iterator_category;
        typedef DepNode                         value_type;
        typedef sdt                             difference_type;
        typedef DepNode*                        pointer;
        typedef DepNode&                        reference;

        NodeIter_()                                         : _node(nullptr) {}
        NodeIter_(Iter&& it)                                : _it(move(it)) { initIt(); }

        NodeIter_& operator++()
        {
            if (!_it._vertex) return *this;
            if (++_nodeIt == _it->nodes().end()) { ++_it; initIt(); }
            else _node = *_nodeIt;
            return *this;
        }

        NodeIter_ operator++(int)                           { auto tmp = *this; ++*this; return tmp; }

        bool operator==(const NodeIter_& rhs) const         { return _node == rhs._node; }
        bool operator!=(const NodeIter_& rhs) const         { return !operator==(rhs); }

        reference operator*() const                         { assert(_node); return *_node; }
        pointer operator->() const                          { return &operator*(); }

        /// Reset iterator to begin at vertex in graph
        void reset(optional<const Key&> start = optnull, DepType type = DepType::out)
                                                            { _it.reset(start, type); initIt(); }
        /// Skip the current vertex's edges on next step of this iterator
        void skipEdges()                                    { _it._skipEdges(); }
        /// Get the current vertex
        Vertex_& vertex() const                             { return *_it; }

    private:
        void initIt()                                       { _node = _it._vertex ? *(_nodeIt = _it->nodes().begin()) : nullptr; }

        Iter _it;
        typename NodeList::const_iterator _nodeIt;
        DepNode* _node;
    };
    typedef NodeIter_<Vertex> NodeIter;
    typedef NodeIter_<const Vertex> NodeConstIter;

    /// Get a depth-first pre-order range over nodes starting from a vertex.  If start vertex is null then range is over all nodes in graph.
    Range_<NodeConstIter, NodeConstIter> rangeNode(optional<const Key&> start = optnull, DepType type = DepType::out) const   { return honey::range(NodeConstIter(ConstIter(*this, start, type)), NodeConstIter()); }
    Range_<NodeIter, NodeIter>           rangeNode(optional<const Key&> start = optnull, DepType type = DepType::out)         { return honey::range(NodeIter(Iter(*this, start, type)), NodeIter()); }

    /// Check if vertex depends on another vertex
    bool depends(const Key& vertex, const Key& dependency, DepType type = DepType::out) const
    {
        auto range = this->range(vertex, type);
        for (auto it = begin(range); it != end(range); ++it) if (it->keys().count(dependency)) return true;
        return false;
    }
    /// Check if vertex depends on node
    bool depends(const Key& vertex, const DepNode& dependency, DepType type = DepType::out) const
    {
        auto range = this->range(vertex, type);
        return find(range, [&](auto& e) { return e.nodes().count(&dependency); }) != end(range);
    }

    /// Get a vertex which contains a list of cyclical (strongly connected) dependency nodes.
    /**
      * Call condense() prior to this to merge each group of cyclical nodes into a single vertex.
      */ 
    const Vertex* vertex(const Key& key) const
    {
        auto itMap = _vertexMap.find(key);
        return itMap != _vertexMap.end() && !itMap->second->isPhantom() ? itMap->second : nullptr;
    }
    Vertex* vertex(const Key& key)
    {
        auto itMap = _vertexMap.find(key);
        return itMap != _vertexMap.end() && !itMap->second->isPhantom() ? itMap->second : nullptr;
    }

    /// Condense directed graph into directed acyclic graph.  Useful for finding dependency cycles and optimizing searches.
    /**
      * The condensation is done by finding all strongly connected subgraphs and merging each subgraph into one vertex.
      * All nodes can still be added/removed normally after condensation, and condensation can be done repeatedly.
      * If a node is removed after condensation and it belongs to a merged vertex, then the vertex will be decomposed
      * back into its constituent nodes.
      */ 
    void condense()
    {
        CondenseData data;
        VertexLinkList linkList;

        for (auto& vertex : values(_vertexMap))
        {
            assert(data.stackS.empty() && data.stackP.empty(), "Condense algorithm failure");
            linkList.clear();
            linkList.push_back(mtmap(   k_vertex() = vertex,
                                        k_linkMap() = &vertex->linkMap(DepType::out),
                                        k_linkIt() = vertex->linkMap(DepType::out).begin() ));
            condense(data, linkList);
        }

        for (auto& e : data.mergeMap)
        {
            auto& mergeVertex = *e.first;
            auto& oldVertexList = e.second;

            //Build merged vertex links
            for (auto i : honey::range(DepNode::depTypeMax))
            {
                auto type = DepType(i);
                auto depTypeOpp = DepNode::depTypeOpp(type);

                //Loop through old vertices
                for (auto& e : oldVertexList)
                {
                    auto& oldVertex = *e;
                    for (auto& e : oldVertex.linkMap(type))
                    {
                        auto& vertex = *e.first;

                        //Ignore link to self
                        if (oldVertexList.count(&vertex)) continue;

                        //If the vertex at the other end is also being merged then link to that merged vertex instead
                        auto pVertex = &vertex;
                        auto itMerged = data.mergeMapR.find(pVertex);
                        if (itMerged != data.mergeMapR.end()) pVertex = itMerged->second;

                        //Add link to merged vertex
                        mergeVertex.add(type, pVertex, e.second);

                        //Merge links at other end
                        //If the vertex at the other end is also being merged then its links are clean
                        if (pVertex != &vertex) continue;

                        //Get old link
                        auto& linkMap = vertex.linkMap(depTypeOpp);
                        auto itOld = linkMap.find(&oldVertex);
                        assert(itOld != linkMap.end(), "Old link not found, problem with algorithm");
                        //Move old link into merged link
                        vertex.add(depTypeOpp, &mergeVertex, itOld->second);
                        linkMap.erase(itOld);
                    }
                }
            }

            //Remap to merged vertex
            for (auto& e : mergeVertex.keyList) { _vertexMap[e] = &mergeVertex; }

            //Delete old vertices
            for (auto& e : oldVertexList)
            {
                auto& oldVertex = *e;
                //Clear key list so delete doesn't unmap merged vertex
                oldVertex.keyList.clear();
                deleteVertex(oldVertex);
            }
        }
    }

private:
    Vertex& createVertex()
    {
        //Add to allocated list
        _vertexList.resize(_vertexList.size()+1);
        //Set up self iterator for fast removal
        _vertexList.back().itAlloc = --_vertexList.end();
        return _vertexList.back();
    }

    /// Create vertex if it doesn't exist
    Vertex& createVertex(const Key& key)
    {
        auto itMap = _vertexMap.find(key);
        if (itMap == _vertexMap.end()) itMap = mapVertex(createVertex(), key);
        return *itMap->second;
    }

    ///Insert vertex into map for key
    typename VertexMap::iterator mapVertex(Vertex& vertex, const Key& key)
    {
        vertex.keyList.insert(key); 
        return _vertexMap.insert(make_pair(key, &vertex)).first;
    }

    void deleteVertex(Vertex& vertex)
    {
        //Remove all keys from map
        for (auto& e : vertex.keyList) { _vertexMap.erase(e); }
        //Remove from allocated list
        _vertexList.erase(vertex.itAlloc);
    }

    /**
      * \verbatim
      * Gabow's strongly connected components algorithm
      *
      * Set the preorder number of v to C, and increment C.
      * Push v onto S and also onto P.
      * For each edge from v to a neighboring vertex w:
      *     If the preorder number of w has not yet been assigned, recursively search w;
      *     Otherwise, if w has not yet been assigned to a strongly connected component:
      *         Repeatedly pop vertices from P until the top element of P has a preorder number less than or equal to the preorder number of w.
      * If v is the top element of P:
      *     Pop vertices from S until v has been popped, and assign the popped vertices to a new component.
      *     Pop v from P.
      * \endverbatim
      */
    void condense(CondenseData& data, const VertexLinkList& linkList)
    {
        auto vertex = linkList.back()[k_vertex()];
        if (!data.preOrdMap.insert(make_pair(vertex, data.preOrd)).second) return;
        ++data.preOrd;
        data.stackS.push_back(vertex);
        data.stackP.push_back(vertex);

        VertexLinkList recurseList;

        for (auto& e : linkList)
        {
            auto links = honey::range(e[k_linkIt()], e[k_linkMap()]->end());
            for (auto& linkVertex : keys(links))
            {
                auto itPreOrd = data.preOrdMap.find(linkVertex);
                if (itPreOrd == data.preOrdMap.end())
                {
                    //Move along out links
                    recurseList.clear();
                    recurseList.push_back(mtmap(k_vertex() = linkVertex,
                                                k_linkMap() = &linkVertex->linkMap(DepType::out),
                                                k_linkIt() = linkVertex->linkMap(DepType::out).begin() ));
                    condense(data, recurseList);
                }
                else if (!data.assignedList.count(linkVertex))
                {
                    auto linkPreOrd = itPreOrd->second;
                    while (data.preOrdMap.find(data.stackP.back())->second > linkPreOrd)
                    {
                        data.stackP.pop_back();
                        assert(!data.stackP.empty(), "Condense algorithm failure");
                    }
                }
            }
        }

        if (data.stackP.back() == vertex)
        {
            //Only merge if there is more than one node in merge list
            if (data.stackS.back() != vertex)
            {
                //Merge strongly connected vertices
                auto& mergeVertex = createVertex();

                Vertex* assignVertex;
                do
                {
                    assignVertex = data.stackS.back();
                    data.stackS.pop_back();

                    data.assignedList.insert(assignVertex);
                    data.mergeMap[&mergeVertex].insert(assignVertex);
                    data.mergeMapR[assignVertex] = &mergeVertex;

                    //Merge nodes into one node

                    //Merge references
                    for (auto& e : assignVertex->nodeList)
                    {
                        //Ensure that merged references are unique.  If this assert fails then there is a problem with the class' algorithms.
                        assert(!mergeVertex.nodeList.count(e), sout() << "Duplicate reference during condense merge. Node: " << e->getKey());
                        mergeVertex.nodeList.insert(e);
                    }

                    //Merge keys
                    for (auto& e : assignVertex->keyList)
                    {
                        //Ensure that merged keys are unique.  If this assert fails then there is a problem with the class' algorithms.
                        assert(!mergeVertex.keyList.count(e), sout() << "Duplicate key during condense merge. Node: " << e);
                        mergeVertex.keyList.insert(e);
                    }

                } while (assignVertex != vertex);
            }
            else
            {
                //There was only one node in merge list, ignore it
                data.assignedList.insert(data.stackS.back());
                data.stackS.pop_back();
            }

            data.stackP.pop_back();
        }
    }
    
    /// Decomposes a previously merged vertex back into its constituent nodes
    void decondense(Vertex& mergeVertex)
    {
        //Unmap merged vertex
        for (auto& e : mergeVertex.keyList) { _vertexMap.erase(e); }
        //Re-add component nodes
        for (auto& e : mergeVertex.nodeList) { add(*e); }

        //Loop through links and update links on other side
        for (auto i : honey::range(DepNode::depTypeMax))
        {
            auto type = DepType(i);
            auto depTypeOpp = DepNode::depTypeOpp(type);

            for (auto& e : keys(mergeVertex.linkMap(type)))
            {
                auto& vertex = *e;
                //Remove link to merged vertex
                vertex.linkMap(depTypeOpp).erase(&mergeVertex);
                //Loop through nodes and re-add links that have the same type and reference a component vertex
                for (auto& e : vertex.nodeList)
                {
                    auto& node = *e;
                    for (auto& e : node.deps())
                    {
                        auto type = e.second;
                        if (type != depTypeOpp) continue;

                        auto& key = e.first;
                        if (!mergeVertex.keyList.count(key)) continue;

                        //Create phantom vertex if it doesn't exist
                        auto& depVertex = createVertex(key);

                        switch (type)
                        {
                        case DepType::out:
                            vertex.add(DepType::out, &depVertex);
                            depVertex.add(DepType::in, &vertex);
                            break;

                        case DepType::in:
                            vertex.add(DepType::in, &depVertex);
                            depVertex.add(DepType::out, &vertex);
                            break;
                        }
                    }
                }
            }
        }

        //Clear key list so delete doesn't unmap component vertices
        mergeVertex.keyList.clear();
        deleteVertex(mergeVertex);
    }

    VertexList _vertexList;
    VertexMap _vertexMap;
};

}
