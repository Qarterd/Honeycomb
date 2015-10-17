// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Alge/Alge.h"

namespace honey
{

/// A contiguous region of referenced (not owned by object) memory
/**
  * A buffer is a light wrapper around a pointer, as such the constness of a buffer object
  * does not affect the mutability of its referenced memory. Also, for readability it
  * is better to pass a buffer by value instead of by const reference
  * (eg. declare `Buffer<T>` instead of `const Buffer<T>&`, similar to how one would declare `T*` instead of `T* const&`).
  */
template<class T>
class Buffer
{
public:
    typedef T           value_type;
    typedef szt         size_type;
    typedef sdt         difference_type;
    typedef T&          reference;
    typedef const T&    const_reference;
    typedef T*          pointer;
    typedef const T*    const_pointer;
    typedef T*          iterator;
    typedef std::reverse_iterator<iterator>         reverse_iterator;
    
    /// Construct empty buffer with null data and zero size
    Buffer()                                        { reset(); }
    /// Construct from memory reference and number of contiguous elements in region
    Buffer(T* data, szt size)                       { reset(data, size); }
    /// Construct from vector with compatible memory reference type
    template<class U>
    Buffer(vector<U>& list)                         { reset(list.data(), list.size()); }
    template<class U>
    Buffer(const vector<U>& list)                   { reset(list.data(), list.size()); }
    /// Construct from array with compatible memory reference type
    template<class U, szt N>
    Buffer(array<U,N>& list)                        { reset(list.data(), list.size()); }
    template<class U, szt N>
    Buffer(const array<U,N>& list)                  { reset(list.data(), list.size()); }
    Buffer(const Buffer& rhs)                       { operator=(rhs); }
    Buffer(Buffer&& rhs)                            { operator=(move(rhs)); }
    
    Buffer& operator=(const Buffer& rhs)            { reset(rhs._data, rhs._size); return *this; }
    Buffer& operator=(Buffer&& rhs)                 { reset(rhs._data, rhs._size); rhs.reset(); return *this; }
    
    /// Access element with bounds checking, throws std::out_of_range
    T& at(szt pos) const                            { if (pos >= _size) throw std::out_of_range("invalid buffer index"); return _data[pos]; }
    /// Access element without bounds checking
    T& operator[](szt pos) const                    { return _data[pos]; }
    /// Returns the first element without bounds checking
	T& front() const                                { return _data[0]; }
	/// Returns the last element without bounds checking
    T& back() const                                 { return _data[_size - 1]; }
    /// Get the referenced memory
    T* data() const                                 { return _data; }
    /// Get a view of this memory region in range `[pos, pos+size)`
    /**
      * The view will be clipped against the bounds of this region, possibly resulting in an empty view.
      */
    Buffer slice(szt pos, optional<szt> size = optnull) const   { szt begin = Alge::clamp(pos, 0, _size), end = size ? Alge::clamp(pos + *size, 0, _size) : _size; return Buffer(_data + begin, end - begin); }

    /// Returns an iterator to the beginning
    T* begin() const                                { return _data; }
    /// Returns an iterator to the end
    T* end() const                                  { return _data + _size; }
    /// Returns a reverse iterator to the beginning
    reverse_iterator rbegin() const                 { return reverse_iterator(_data + _size); }
    /// Returns a reverse iterator to the end
    reverse_iterator rend() const                   { return reverse_iterator(_data); }
    
    /// Checks whether the buffer does not have any elements
    bool empty() const                              { return !_data || !_size; }
    /// Checks whether the buffer has elements
    explicit operator bool() const                  { return !empty(); }
    /// Returns the number of elements
	szt size() const                                { return _size; }
    
    /// Check if buffers reference same memory region
    bool operator==(const Buffer& rhs) const        { return _data == rhs._data && _size == rhs._size; }
	bool operator!=(const Buffer& rhs) const        { return !operator==(rhs); }
    
    /// Set data to null with zero size
    void reset()                                    { _data = nullptr; _size = 0; }
    /// Set new data and size
    void reset(T* data, szt size)                   { _data = data; _size = size; }
    
    /// Implicitly convert to const buffer
    operator Buffer<const T>() const                { return Buffer<const T>(_data, _size); }
    
private:
    T* _data;
    szt _size;
};

}
