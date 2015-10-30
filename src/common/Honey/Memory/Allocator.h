// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Meta.h"

/// Memory-management and allocators
/**
  * \defgroup Memory    Memory Util
  */
/// @{

/// Global `new_`, use in place of `new` keyword to provide allocator with debug info
#ifdef DEBUG
    #define new_                                new (__FILE__, __LINE__)
#else
    #define new_                                new
#endif

inline void* operator new(size_t size, const char* srcFile, int srcLine)    { mt_unused(srcFile); mt_unused(srcLine); return operator new(size); }
inline void* operator new[](size_t size, const char* srcFile, int srcLine)  { mt_unused(srcFile); mt_unused(srcLine); return operator new(size); }
/// @}

namespace honey
{

/// \addtogroup Memory
/// @{

/// Allocate memory for `count` number of objects.  Objects are not constructed.
template<class T>
T* alloc(szt count = 1)                         { return static_cast<T*>(operator new(sizeof(T)*count)); }
/// Deallocate memory and set pointer to null. Object is not destroyed.
template<class T>
void free(T*& p)                                { if (!p) return; operator delete(p); p = nullptr; }
template<class T>
void free(T* const& p)                          { if (!p) return; operator delete(p); }

/// Align a pointer to the previous byte boundary `bytes`. Does nothing if p is already on boundary.  Alignment must be a power of two.
template<class T>
T* alignFloor(T* p, szt bytes)                  { return reinterpret_cast<T*>(intptr_t(p) & ~(bytes-1)); }
/// Align a pointer to the next byte boundary `bytes`. Does nothing if p is already on boundary.  Alignment must be a power of two.
template<class T>
T* alignCeil(T* p, szt bytes)                   { return alignFloor(reinterpret_cast<T*>(intptr_t(p) + bytes-1), bytes); }

/// Allocate memory with alignment.  Alignment must be a power of two.  Allocator element type must be int8.
template<class T, class Alloc>
T* allocAligned(szt count, szt align_, Alloc&& a)
{
    int8* base = a.allocate(sizeof(sdt) + align_-1 + sizeof(T)*count);
    if (!base) return nullptr;
    int8* p = alignCeil(base+sizeof(sdt), align_);
    *reinterpret_cast<sdt*>(p-sizeof(sdt)) = p - base;
    return reinterpret_cast<T*>(p);
}
/// Allocate memory with alignment using default allocator
template<class T>
T* allocAligned(szt count, szt align)           { return allocAligned<T>(count, align, std::allocator<int8>()); }

/// Deallocate aligned memory.  Allocator element type must be int8.
template<class T, class Alloc>
void freeAligned(T* p, Alloc&& a)
{
    if (!p) return;
    int8* p_ = reinterpret_cast<int8*>(p);
    int8* base = p_ - *reinterpret_cast<sdt*>(p_ - sizeof(sdt));
    a.deallocate(base, 1);
}
/// Deallocate aligned memory using default allocator
template<class T>
void freeAligned(T* p)                          { freeAligned(p, std::allocator<int8>()); }


/// Destruct object, free memory and set pointer to null
template<class T>
void delete_(T*& p)                             { delete p; p = nullptr; }
template<class T>
void delete_(T* const& p)                       { delete p; }

/// Destruct `count` number of objects, free memory using allocator and set pointer to null
template<class T, class Alloc>
void delete_(T*& p, Alloc&& a, szt count = 1)       { if (!p) return; for (szt i = 0; i < count; ++i) a.destroy(p+i); a.deallocate(p, count); p = nullptr; }
template<class T, class Alloc>
void delete_(T* const& p, Alloc&& a, szt count = 1) { if (!p) return; for (szt i = 0; i < count; ++i) a.destroy(p+i); a.deallocate(p, count); }

/// Destruct all array objects, free memory and set pointer to null
template<class T>
void deleteArray(T*& p)                         { delete[] p; p = nullptr; }
template<class T>
void deleteArray(T* const& p)                   { delete[] p; }

/// std::allocator compatible allocator
/**
  * Subclass must define:
  * - default/copy/copy-other ctors
  * - pointer allocate(size_type n, const void* hint = 0)
  * - pointer allocate(size_type n, const char* srcFile, int srcLine, const void* hint = 0)
  * - void deallocate(pointer p, size_type n)
  */
template<template<class> class Subclass, class T>
class Allocator
{
public:
    typedef T           value_type;
    typedef T*          pointer;
    typedef T&          reference;
    typedef const T*    const_pointer;
    typedef const T&    const_reference;
    typedef szt         size_type;
    typedef sdt         difference_type;

    pointer address(reference x) const                      { return &x; }
    const_pointer address(const_reference x) const          { return &x; }
    size_type max_size() const                              { return std::numeric_limits<size_type>::max(); }
    template<class U, class... Args>
    void construct(U* p, Args&&... args)                    { new ((void*)p) U(forward<Args>(args)...); }
    template<class U>
    void destroy(U* p)                                      { p->~U(); }
    template<class U> struct rebind                         { typedef Subclass<U> other; };
    bool operator==(const Subclass<T>&) const               { return true; }
    bool operator!=(const Subclass<T>&) const               { return false; }

protected:
    Subclass<T>& subc()                                     { return static_cast<Subclass<T>&>(*this); }
    const Subclass<T>& subc() const                         { return static_cast<const Subclass<T>&>(*this); }
};

/// Objects that inherit from this class will use `Alloc` for new/delete ops
template<template<class> class Alloc>
class AllocatorObject
{
public:
    template<class T> using Allocator = Alloc<T>;

    void* operator new(szt size)                                            { return _alloc.allocate(size); }
    /// Placement new, does nothing
    void* operator new(szt, void* ptr)                                      { return ptr; }
    void* operator new(szt size, const char* srcFile, int srcLine)          { return _alloc.allocate(size, srcFile, srcLine); }
    
    void* operator new[](szt size)                                          { return _alloc.allocate(size); }
    /// Placement new, does nothing
    void* operator new[](szt, void* ptr)                                    { return ptr; }
    void* operator new[](szt size, const char* srcFile, int srcLine)        { return _alloc.allocate(size, srcFile, srcLine); }
    
    void operator delete(void* p, szt size)                                     { _alloc.deallocate(static_cast<uint8*>(p), size); }
    /// Delete for placement new, does nothing
    void operator delete(void* p, void* ptr)                                    {}
    void operator delete(void* p, szt size, const char* srcFile, int srcLine)   { _alloc.deallocate(static_cast<uint8*>(p), size); }

    void operator delete[](void* p, szt size)                                   { _alloc.deallocate(static_cast<uint8*>(p), size); }
    /// Delete for placement new, does nothing
    void operator delete[](void* p, void* ptr)                                  {}
    void operator delete[](void* p, szt size, const char* srcFile, int srcLine) { _alloc.deallocate(static_cast<uint8*>(p), size); }

private:
    static Alloc<uint8> _alloc;
};
template<template<class> class Alloc> Alloc<uint8> AllocatorObject<Alloc>::_alloc;

/// Returns T::Allocator<T> if available, otherwise std::allocator<T>
template<class T, class = std::true_type>
struct defaultAllocator                         { typedef std::allocator<T> type; };
template<class T>
struct defaultAllocator<T[], std::true_type>    { typedef std::allocator<T> type; };
template<class T>
struct defaultAllocator<T, typename mt::True<typename T::template Allocator<T>>::type>
                                                { typedef typename T::template Allocator<T> type; };

/// Functor to delete a pointer
template<class T, class Alloc = typename defaultAllocator<T>::type>
struct finalize
{
    finalize(Alloc a = Alloc(), szt count = 1)  : a(move(a)), count(count) {}
    void operator()(T*& p)                      { delete_(p,a,count); }
    void operator()(T* const& p)                { delete_(p,a,count); }
    Alloc a;
    szt count;
};
/// Specialization for array.
/** \note array placement-new can't be used with custom allocators as its overhead is compiler-specific */
template<class T>
struct finalize<T[], std::allocator<T>>
{
    void operator()(T*& p)                      { deleteArray(p); }
    void operator()(T* const& p)                { deleteArray(p); }
};
/// Specialization for void
template<>
struct finalize<void, std::allocator<void>>
{
    void operator()(void*& p)                   { free(p); }
    void operator()(void* const& p)             { free(p); }
};

/// @}

}
