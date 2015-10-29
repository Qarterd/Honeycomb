// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Thread/LockFree/FreeList.h"

namespace honey { namespace lockfree
{

/// Lock-free unordered map. Uses auto-expanding freelist allocator so memory is only reclaimed upon destruction.
/**
  * Based on the paper: "Split-Ordered Lists - Lock-free Resizable Hash Tables", Shalev, Shavit - 2006
  */
template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class UnorderedMap : mt::NoCopy
{
    typedef typename FreeList<T>::Handle Handle;
    typedef typename FreeList<T>::TaggedHandle TaggedHandle;
    
    ///Use first bit of tag for delete mark
    struct MarkedHandle : TaggedHandle
    {
        using typename TaggedHandle::Int;
        using TaggedHandle::tag;
        
        MarkedHandle() = default;
        MarkedHandle(Handle handle, Int tag, bool mark)     : TaggedHandle(handle, (tag<<1) | Int(mark)) {}
        
        Int nextTag() const                                 { return (tag>>1)+1; }
        bool mark() const                                   { return tag & true; }
        void mark(bool b)                                   { tag = (tag & (~Int(0)<<1)) | b; }
    };
    
    struct Node
    {
        //init without overwriting previous tag
        template<class Pair>
        Node(Pair&& pair) :
            key(forward<typename Pair::first_type>(pair.first)),
            soKey(0),
            val(forward<typename Pair::second_type>(pair.second))
        {
            next.store(MarkedHandle(nullptr, next.load(atomic::Order::relaxed).nextTag(), false), atomic::Order::relaxed);
        }
        
        Key key;
        szt soKey;
        T val;
        Atomic<MarkedHandle> next;
    };
    
    typedef FreeList<Node> FreeList;
    
public:
    typedef T value_type;
    
    UnorderedMap(szt capacity = 0, const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual()) :
        _freeList(capacity),
        _hash(hash),
        _equal(equal),
        _segmentCount(0),
        _bucketCount(0),
        _size(0),
        _maxLoadFactor(4)                           { expand(true); }

    ~UnorderedMap()                                 { clear(); }
    
    /// Ensure that enough storage is allocated for a number of elements
    void reserve(szt capacity)                      { _freeList.reserve(capacity); }
    /// The number of elements for which storage is allocated
    szt capacity() const                            { return _freeList.capacity(); }
    
    /// Insert new key-value pair into the map. Returns true on success, false if element with key already exists.
    template<class Pair>
    bool insert(Pair&& pair)
    {
        Node* node = _freeList.construct(forward<Pair>(pair));
        szt hash = _hash(node->key);
        node->soKey = BitOp::reverse(hash) | true; //regular key has LSB set
        auto& bucket = getBucket(hash % _bucketCount);
        if (!get<0>(list_insert(bucket, *node))) { _freeList.destroy(node); return false; }
        ++_size;
        if (load_factor() > max_load_factor()) expand();
        return true;
    }
    
    /// Remove element with key from the map and copy its value into `val`. Returns true on success, false if not found.
    bool erase(const Key& key, optional<T&> val = optnull)
    {
        szt hash = _hash(key);
        szt soKey = BitOp::reverse(hash) | true; //regular key has LSB set
        auto& bucket = getBucket(hash % _bucketCount);
        if (!list_delete(bucket, key, soKey, val)) return false;
        --_size;
        return true;
    }
    
    /// Remove all elements
    void clear()
    {
        while (true)
        {
            Node* prev = &getBucket(0);
            MarkedHandle cur = prev->next.load(atomic::Order::acquire);
            while (true)
            {
                if (!cur) return;
                MarkedHandle next = _freeList.deref(cur)->next.load(atomic::Order::acquire);
                auto ckey = _freeList.deref(cur)->key;
                auto csoKey = _freeList.deref(cur)->soKey;
                if (prev->next.load(atomic::Order::acquire) != cur) break; //try again
                if (!next.mark())
                {
                    //regular key has LSB set
                    if (csoKey & true)
                    {
                        erase(ckey);
                        cur = MarkedHandle(next, cur.nextTag(), false);
                    }
                    else
                    {
                        prev = _freeList.deref(cur);
                        cur = next;
                    }
                }
                else
                {
                    auto next_ = MarkedHandle(next, cur.nextTag(), false);
                    if (prev->next.cas(next_, cur))
                    {
                        _freeList.destroy(_freeList.deref(cur));
                        cur = next_;
                    }
                    else
                        break; //try again
                }
            }
        }
    }
    
    /// Find element with key and copy its value into `val`. Returns true on success, false if not found.
    bool find(const Key& key, optional<T&> val = optnull) const
    {
        szt hash = _hash(key);
        szt soKey = BitOp::reverse(hash) | true; //regular key has LSB set
        auto& bucket = getBucket(hash % _bucketCount);
        
        bool found; Node* prev; MarkedHandle cur, next;
        do
        {
            tie(found, prev, cur, next) = list_find(bucket, key, soKey);
            if (!found) return false;
            //ensure val we read is consistent with prev, otherwise we could end up returning a destroyed value
            if (val) val = _freeList.deref(cur)->val;
        } while (prev->next.load(atomic::Order::acquire) != cur);
        return true;
    }
    
    /// Return number of elements with matching key (either 0 or 1)
    szt count(const Key& key) const                 { return find(key); }
    
    /// Check whether the map does not contain any elements
    bool empty() const                              { return !_size; }
    /// The number of elements in the map
    szt size() const                                { return _size; }
    /// The number of buckets. A bucket is a slot in the internal hash table to which elements are assigned based on their key hash.
    szt bucket_count() const                        { return _bucketCount; }
    /// The current load factor. The load factor is the ratio between the number of elements and the number of buckets.
    float load_factor() const                       { return _bucketCount ? float(_size) / _bucketCount : _size ? numeral<float>().inf() : 0; }
    /// Get the max load factor. The internal hash table will expand when the load factor is above the max load factor.
    float max_load_factor() const                   { return _maxLoadFactor; }
    /// Set the max load factor
    void max_load_factor(float f)                   { _maxLoadFactor = f; expand(); }
    
private:
    ///Expand exponentially until under load factor
    void expand(bool init = false)
    {
        SpinLock::Scoped _(_lock);
        while (init || load_factor() > max_load_factor())
        {
            szt count = _segmentCount ? (1<<_segmentCount) : 2;
            auto buckets = make_unique<Atomic<MarkedHandle>[]>(count);
            std::fill_n(buckets.get(), count, MarkedHandle());
            assert(_segmentCount < _segments.size(), "Max segments reached");
            _segments[_segmentCount] = move(buckets);
            ++_segmentCount;
            _bucketCount += count;
            if (init) break;
        }
    }
    
    Node& getBucket(szt i) const
    {
        szt segment = BitOp::log2Floor(i ? i : 1);
        Node* parent = i ? &getBucket(~(1<<segment) & i) : nullptr; //parent is index with MSB unset
        auto& bucket = _segments[segment][i >= 2 ? i - (szt(1)<<segment) : i];
        MarkedHandle old = bucket.load(atomic::Order::acquire);
        if (!old)
        {
            //init bucket
            Node* node = _freeList.construct(make_pair(Key(), T()));
            node->soKey = BitOp::reverse(i) & (~szt(0) << 1); //bucket key has LSB unset
            if (parent)
            {
                //insert into position starting search at parent bucket
                bool inserted; MarkedHandle cur;
                tie(inserted, cur) = list_insert(*parent, *node);
                if (!inserted) _freeList.destroy(node);
                //try to update bucket pointer
                bucket.cas(MarkedHandle(cur, old.nextTag(), false), old);
            }
            else
            {
                //first bucket, try to update bucket pointer
                if (!bucket.cas(MarkedHandle(_freeList.handle(node), old.nextTag(), false), old))
                    _freeList.destroy(node);
            }
            old = bucket.load(atomic::Order::acquire);
        }
        return *_freeList.deref(old);
    }
    
    //tuple<inserted, cur>
    tuple<bool, MarkedHandle> list_insert(Node& head, Node& node) const
    {
        while (true)
        {
            bool found; Node* prev; MarkedHandle cur, next;
            tie(found, prev, cur, next) = list_find(head, node.key, node.soKey);
            if (found) return make_tuple(false, cur);
            node.next.store(MarkedHandle(cur, node.next.load(atomic::Order::relaxed).nextTag(), false), atomic::Order::relaxed);
            auto node_ = MarkedHandle(_freeList.handle(&node), cur.nextTag(), false);
            if (prev->next.cas(node_, cur)) return make_tuple(true, node_);
        }
    }
    
    bool list_delete(Node& head, const Key& key, szt soKey, optional<T&> val = optnull)
    {
        bool found; Node* prev; MarkedHandle cur, next;
        do
        {
            tie(found, prev, cur, next) = list_find(head, key, soKey);
            if (!found) return false;
            if (val) val = _freeList.deref(cur)->val; //get val before cas-ing mark otherwise another thread could delete it
        } while (!_freeList.deref(cur)->next.cas(MarkedHandle(next, next.nextTag(), true), next));
        
        if (prev->next.cas(MarkedHandle(next, cur.nextTag(), false), cur))
            _freeList.destroy(_freeList.deref(cur));
        else
            list_find(head, key, soKey);
        return true;
    }
    
    //tuple<found, prev, cur, next>
    tuple<bool, Node*, MarkedHandle, MarkedHandle> list_find(Node& head, const Key& key, szt soKey) const
    {
        while (true)
        {
            Node* prev = &head;
            MarkedHandle cur = prev->next.load(atomic::Order::acquire);
            while (true)
            {
                if (!cur) return make_tuple(false, prev, cur, MarkedHandle());
                MarkedHandle next = _freeList.deref(cur)->next.load(atomic::Order::acquire);
                auto ckey = _freeList.deref(cur)->key;
                auto csoKey = _freeList.deref(cur)->soKey;
                if (prev->next.load(atomic::Order::acquire) != cur) break; //try again
                if (!next.mark())
                {
                    if (csoKey == soKey && _equal(ckey, key)) return make_tuple(true, prev, cur, next);
                    if (csoKey > soKey) return make_tuple(false, prev, cur, next);
                    prev = _freeList.deref(cur);
                    cur = next;
                }
                else
                {
                    auto next_ = MarkedHandle(next, cur.nextTag(), false);
                    if (prev->next.cas(next_, cur))
                    {
                        _freeList.destroy(_freeList.deref(cur));
                        cur = next_;
                    }
                    else
                        break; //try again
                }
            }
        }
    }
    
    mutable FreeList    _freeList;
    Hash                _hash;
    KeyEqual            _equal;
    SpinLock            _lock;
    array<UniquePtr<Atomic<MarkedHandle>[]>, numeral<uint64>().sizeBits> _segments; ///< this array is small as segments grow exponentially
    uint8               _segmentCount;
    Atomic<szt>         _bucketCount;
    Atomic<szt>         _size;
    Atomic<float>       _maxLoadFactor;
};

} }
