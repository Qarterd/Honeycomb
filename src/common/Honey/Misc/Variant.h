// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Misc/Exception.h"

namespace honey
{

struct VariantError : Exception                             { EXCEPTION(VariantError) };

/** \cond */
namespace priv
{
    template<class Subclass, int id, class... Types> class variant;

    /// Base class of honey::variant
    template<class Subclass, int id, class Type, class... Types>
    class variant<Subclass, id, Type, Types...> : public variant<Subclass, id+1, Types...>
    {
        typedef variant<Subclass, id+1, Types...> Super;
        typedef typename std::remove_const<Type>::type Type_nonconst;
    public:
        operator Type&()                                    { return this->subc().template get<Type>(); }
        operator const Type&() const                        { return this->subc().template get<const Type>(); }
        
    protected:
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_constructible<Type>::value, int>::type=0>
        void construct(const Type_nonconst& val)            { construct_(val); }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_constructible<Type>::value, int>::type=0>
        void construct(Type_nonconst& val)                  { construct_(val); }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_move_constructible<Type>::value, int>::type=0>
        void construct(Type_nonconst&& val)                 { construct_(move(val)); }
        template<class T>
        void construct(T&& val)                             { Super::construct(forward<T>(val)); }
        
        template<class... Args, typename std::enable_if<std::is_constructible<Type, Args...>::value, int>::type=0>
        void construct_convert(Args&&... args)              { construct_(forward<Args>(args)...); }
        template<class... Args, typename mt::disable_if<std::is_constructible<Type, Args...>::value, int>::type=0>
        void construct_convert(Args&&... args)              { Super::construct_convert(forward<Args>(args)...); }
        
        void destroy()                                      { _id() == id ? _val().~Type() : Super::destroy(); }
    
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_assignable<Type>::value, int>::type=0>
        void assign(const Type_nonconst& val)               { if (_id() != id) assign_new(val); else _val() = val; }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_assignable<Type>::value, int>::type=0>
        void assign(Type_nonconst& val)                     { if (_id() != id) assign_new(val); else _val() = val; }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_move_assignable<Type>::value, int>::type=0>
        void assign(Type_nonconst&& val)                    { if (_id() != id) assign_new(move(val)); else _val() = move(val); }
        template<class T>
        void assign(T&& val)                                { Super::assign(forward<T>(val)); }
        
        template<class T, typename std::enable_if<std::is_assignable<Type&, T>::value && std::is_constructible<Type, T>::value, int>::type=0>
        void assign_convert(T&& val)                        { if (_id() != id) assign_new(forward<T>(val)); else _val() = forward<T>(val); }
        template<class T, typename mt::disable_if<std::is_assignable<Type&, T>::value && std::is_constructible<Type, T>::value, int>::type=0>
        void assign_convert(T&& val)                        { Super::assign_convert(forward<T>(val)); }
        
        template<int id_, int=0>
        struct Type_                                        : Super::template Type_<id_> {};
        template<int _> struct Type_<id, _>                 { typedef Type type; };
        
        template<class R, class Func, class... Args, typename std::enable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args)                   { if (_id() == id) return f(_val(), forward<Args>(args)...); else return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        template<class R, class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args)                   { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        
        template<class R, class Func, class... Args, typename std::enable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args) const             { if (_id() == id) return f(_val(), forward<Args>(args)...); else return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        template<class R, class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args) const             { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        
        template<class Func, class... Args, typename std::enable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args)                { if (_id() == id) f(_val(), forward<Args>(args)...); else Super::visit(forward<Func>(f), forward<Args>(args)...); }
        template<class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args)                { Super::visit(forward<Func>(f), forward<Args>(args)...); }
        
        template<class Func, class... Args, typename std::enable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args) const          { if (_id() == id) f(_val(), forward<Args>(args)...); else Super::visit(forward<Func>(f), forward<Args>(args)...); }
        template<class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args) const          { Super::visit(forward<Func>(f), forward<Args>(args)...); }
        
    private:
        int& _id()                                          { return this->subc()._id; }
        const int& _id() const                              { return this->subc()._id; }
        void* _storage()                                    { return &this->subc()._storage; }
        const void* _storage() const                        { return &this->subc()._storage; }
        Type& _val()                                        { assert(_id() == id, "Value not initialized"); return *reinterpret_cast<Type*>(_storage()); }
        const Type& _val() const                            { assert(_id() == id, "Value not initialized"); return *reinterpret_cast<const Type*>(_storage()); }
        
        template<class... Args>
        void construct_(Args&&... args)                     { _id() = id; new (_storage()) Type(forward<Args>(args)...); }
        
        template<class T> void assign_new(T&& val)          { this->subc().destroy(); construct_(forward<T>(val)); }
    };

    /// Specialization for bounded reference types
    template<class Subclass, int id, class Type, class... Types>
    class variant<Subclass, id, Type&, Types...> : public variant<Subclass, id+1, Types...>
    {
        typedef variant<Subclass, id+1, Types...> Super;
        typedef typename std::remove_const<Type>::type Type_nonconst;
    public:
        operator Type&()                                    { return this->subc().template get<Type>(); }
        operator const Type&() const                        { return this->subc().template get<const Type>(); }
        
    protected:
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_const<Type>::value, int>::type=0>
        void construct(const Type_nonconst& val)            { construct_(val); }
        void construct(Type_nonconst& val)                  { construct_(val); }
        void construct(Type_nonconst&& val)                 { construct_(move(val)); }
        template<class T>
        void construct(T&& val)                             { Super::construct(forward<T>(val)); }
        
        template<class T, typename std::enable_if<std::is_convertible<typename mt::addPtr<T>::type, Type*>::value, int>::type=0>
        void construct_convert(T&& val)                     { construct_(forward<T>(val)); }
        template<class... Args>
        void construct_convert(Args&&... args)              { Super::construct_convert(forward<Args>(args)...); }
        
        void destroy()                                      { if (_id() == id) _ptr() = nullptr; else Super::destroy(); }
        
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_const<Type>::value, int>::type=0>
        void bind(const Type_nonconst& val)                 { bind_(val); }
        void bind(Type_nonconst& val)                       { bind_(val); }
        void bind(Type_nonconst&& val)                      { bind_(move(val)); }
        template<class T>
        void bind(T&& val)                                  { Super::bind(forward<T>(val)); }
        
        template<class T, typename std::enable_if<std::is_convertible<typename mt::addPtr<T>::type, Type*>::value, int>::type=0>
        void bind_convert(T&& val)                          { bind_(forward<T>(val)); }
        template<class T, typename mt::disable_if<std::is_convertible<typename mt::addPtr<T>::type, Type*>::value, int>::type=0>
        void bind_convert(T&& val)                          { Super::bind_convert(forward<T>(val)); }
    
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_assignable<Type>::value, int>::type=0>
        void assign(const Type_nonconst& val)               { _val() = val; }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_copy_assignable<Type>::value, int>::type=0>
        void assign(Type_nonconst& val)                     { _val() = val; }
        template<class _=void, typename std::enable_if<mt::True<_>::value && std::is_move_assignable<Type>::value, int>::type=0>
        void assign(Type_nonconst&& val)                    { _val() = move(val); }
        template<class T>
        void assign(T&& val)                                { Super::assign(forward<T>(val)); }
        
        template<class T, typename std::enable_if<std::is_assignable<Type&, T>::value, int>::type=0>
        void assign_convert(T&& val)                        { _val() = forward<T>(val); }
        template<class T, typename mt::disable_if<std::is_assignable<Type&, T>::value, int>::type=0>
        void assign_convert(T&& val)                        { Super::assign_convert(forward<T>(val)); }
        
        template<int id_, int=0>
        struct Type_                                        : Super::template Type_<id_> {};
        template<int _> struct Type_<id, _>                 { typedef Type& type; };
        
        template<class R, class Func, class... Args, typename std::enable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args)                   { if (_id() == id) return f(_val(), forward<Args>(args)...); else return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        template<class R, class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args)                   { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        
        template<class R, class Func, class... Args, typename std::enable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args) const             { if (_id() == id) return f(_val(), forward<Args>(args)...); else return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        template<class R, class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        R visit(Func&& f, Args&&... args) const             { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
        
        template<class Func, class... Args, typename std::enable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args)                { if (_id() == id) f(_val(), forward<Args>(args)...); else Super::visit(forward<Func>(f), forward<Args>(args)...); }
        template<class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args)                { Super::visit(forward<Func>(f), forward<Args>(args)...); }
        
        template<class Func, class... Args, typename std::enable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args) const          { if (_id() == id) f(_val(), forward<Args>(args)...); else Super::visit(forward<Func>(f), forward<Args>(args)...); }
        template<class Func, class... Args, typename mt::disable_if<mt::isCallable<Func, const Type&, Args...>::value, int>::type=0>
        void visit(Func&& f, Args&&... args) const          { Super::visit(forward<Func>(f), forward<Args>(args)...); }
        
    private:
        int& _id()                                          { return this->subc()._id; }
        const int& _id() const                              { return this->subc()._id; }
        Type*& _ptr()                                       { assert(_id() == id, "Reference not bound"); return reinterpret_cast<Type*&>(this->subc()._storage); }
        Type* const& _ptr() const                           { assert(_id() == id, "Reference not bound"); return reinterpret_cast<Type* const&>(this->subc()._storage); }
        Type& _val()                                        { assert(_ptr(), "Reference not bound"); return *_ptr(); }
        const Type& _val() const                            { assert(_ptr(), "Reference not bound"); return *_ptr(); }
        
        template<class T> void construct_(T&& val)          { _id() = id; _ptr() = &val; }
        template<class T> void bind_(T&& val)               { this->subc().destroy(); construct_(forward<T>(val)); }
    };
    
    /// Tail of base class
    template<class Subclass, int id>
    class variant<Subclass, id>
    {
    protected:
        /// Get the subclass that inherited from this base class
        const Subclass& subc() const                        { return static_cast<const Subclass&>(*this); }
        Subclass& subc()                                    { return static_cast<Subclass&>(*this); }
        
        template<class T>
        void construct(T&& val)                             { subc().construct_convert(forward<T>(val)); }
        template<class... Args>
        void construct_convert(Args&&...)                   { static_assert(!mt::True<Args...>::value, "No bounded types constructible with args"); }
        
        void destroy()                                      { error_("Should not get here, variant destruction failed"); }
        
        template<class T> void bind(T&& val)                { subc().bind_convert(forward<T>(val)); }
        template<class T> void bind_convert(T&&)            { static_assert(!mt::True<T>::value, "No bounded reference types can bind to type"); }
        
        template<class T> void assign(T&& val)              { subc().assign_convert(forward<T>(val)); }
        template<class T> void assign_convert(T&&)          { static_assert(!mt::True<T>::value, "No bounded types assignable to value"); }
        
        struct size_                                        : mt::Value<int, id> {};
        template<int id_> struct Type_                      { static_assert(!mt::True_int<id_>::value, "Invalid type id"); };
        
        template<class R, class Func, class... Args>
        R visit(Func&&, Args&&...)                          { throw_ VariantError() << "Visitor failed to accept active bounded type"; throw; }
        template<class R, class Func, class... Args>
        R visit(Func&&, Args&&...) const                    { throw_ VariantError() << "Visitor failed to accept active bounded type"; throw; }
        
        template<class Func, class... Args>
        void visit(Func&&, Args&&...) {}
        template<class Func, class... Args>
        void visit(Func&&, Args&&...) const {}
    };
}
/** \endcond */


/// Multi-typed value.  A variant is a value of any type from a fixed set of bounded types, the active bounded type may be changed dynamically.
/**
  * Supports const/ref bounded types. A ref must be bound before it can be used, either construct with an object ref or call bind().
  * All assignments operate on the bound object.
  *
  * A variant is implicitly convertible to any of its bounded types.
  * Implicit conversion throws VariantError if the active bounded type is not convertible to the requested type.
  */
template<class... Types>
class variant : public priv::variant<variant<Types...>, 0, Types...>
{
    template<class, int, class...> friend class priv::variant;
    typedef priv::variant<variant, 0, Types...> Super;
    
public:
    /// First default constructible bounded type is set as the value
    variant()                                               { this->construct_convert(); }
    /// Attempts to copy/move construct any bounded type, otherwise first bounded type constructible with `val` is set as the value.
    /**
      * Fails at compile-time if no bounded types are constructible with `val`.
      */
    template<class T>
    variant(T&& val)                                        { this->construct(forward<T>(val)); }
    /// First bounded type constructible with args is set as the value
    template<class... Args>
    variant(Args&&... args)                                 { this->construct_convert(forward<Args>(args)...); }
    variant(const variant& rhs)                             { rhs.visit(ctorCopy(), *this); }
    variant(variant& rhs)                                   { rhs.visit(ctorCopy(), *this); }
    variant(variant&& rhs)                                  { rhs.visit(ctorMove(), *this); }
    
    ~variant()                                              { this->destroy(); }
    
    /// Bind reference to object.  First bounded reference type bindable to `rhs` is set as the value.
    /**
      * A bounded reference type is bindable to T if T* is convertible to BoundedType*.
      */ 
    template<class T> void bind(T&& val)                    { Super::bind(forward<T>(val)); }
    
    /// Attempts to copy/move-assign to any bounded type, otherwise first bounded type assignable to `val` is set as the value.
    /**
      * For a bounded type to be assignable to `val` it must also be constructible with `val`, this is necessary for changing the active bounded type.
      * Fails at compile-time if no bounded types are assignable to `val`.
      *
      * Bounded reference types must be bound to an object before being assigned, this is asserted at runtime.
      */
    template<class T>
    variant& operator=(T&& val)                             { this->assign(forward<T>(val)); return *this; }
    variant& operator=(const variant& rhs)                  { rhs.visit(assignCopy(), *this); return *this; }
    variant& operator=(variant& rhs)                        { rhs.visit(assignCopy(), *this); return *this; }
    variant& operator=(variant&& rhs)                       { rhs.visit(assignMove(), *this); return *this; }
    
    /// Get number of bounded types at compile-time
    struct size_                                            : Super::size_ {};
    /// Get number of bounded types
    int size() const                                        { return size_::value; };
    /// Get bounded type for id
    template<int id>
    struct Type                                             : Super::template Type_<id> {};
    /// Get active bounded type id, range [0, size)
    int type() const                                        { return _id; }
    
    /// Get variant value as type.  Throws VariantError if the active bounded type is not convertible to the requested type.
    template<class T>
    T& get()                                                { return visit<T&>([](T& val) -> T& { return val; }); }
    template<class T>
    const T& get() const                                    { return visit<const T&>([](const T& val) -> const T& { return val; }); }
    
    /// Visit stored value using functor.  Calls functor(stored_value, args...) if such a call is valid and returns the result.
    /**
      * Throws VariantError if the visitor does not accept the active bounded type.
      */
    template<class R, class Func, class... Args>
    R visit(Func&& f, Args&&... args)                       { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
    template<class R, class Func, class... Args>
    R visit(Func&& f, Args&&... args) const                 { return Super::template visit<R>(forward<Func>(f), forward<Args>(args)...); }
    
    /// Visit for void result type.  Does nothing (no throw) if the visitor does not accept the active bounded type.
    template<class Func, class... Args>
    void visit(Func&& f, Args&&... args)                    { Super::visit(forward<Func>(f), forward<Args>(args)...); }
    template<class Func, class... Args>
    void visit(Func&& f, Args&&... args) const              { Super::visit(forward<Func>(f), forward<Args>(args)...); }
    
    friend ostream& operator<<(ostream& os, const variant& rhs)   { rhs.visit(toString(), os); return os; }
    
private:
    typedef typename std::aligned_storage<mt::max<sizeof(Types)...>::value, mt::max<alignof(Types)...>::value>::type Storage;
    
    struct ctorCopy { template<class T> void operator()(T&& val, variant& this_) { this_.construct(val); } };
    struct ctorMove { template<class T> void operator()(T&& val, variant& this_) { this_.construct(move(val)); } };
    
    struct assignCopy { template<class T> void operator()(T&& val, variant& this_) { this_ = val; } };
    struct assignMove { template<class T> void operator()(T&& val, variant& this_) { this_ = move(val); } };
    
    struct toString { template<class T> void operator()(T&& val, ostream& os) { os << val; } };
    
    int _id;
    Storage _storage;
};



template<class... Funcs> struct overload_;

/// An overloaded visitor functor.  \see overload() to create.
/**
  *
  * Example:
  *
  *     int res = variant.visit<int>(overload
  *     (
  *         [](char) { return 0; },
  *         [](String) { return 1; }
  *     ));
  */
template<class Func, class... Funcs>
struct overload_<Func, Funcs...> : Func, overload_<Funcs...>
{
    typedef overload_<Funcs...> Super;
    overload_(Func&& f, Funcs&&... fs)                      : Func(forward<Func>(f)), Super(forward<Funcs>(fs)...) {}
};

template<> struct overload_<> {};

/// Create an overloaded visitor functor
template<class... Funcs>
overload_<Funcs...> overload(Funcs&&... fs)                 { return overload_<Funcs...>(forward<Funcs>(fs)...); }

}


