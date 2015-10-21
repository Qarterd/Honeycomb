// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Stream.h"

namespace honey
{

/// Use in place of `throw` keyword to throw a honey::Exception object polymorphically and provide debug info
#ifndef FINAL
    #define throw_                      Exception::Raiser() ^ Exception::Source(__FUNC__, __FILE__, __LINE__) << 
#else
    #define throw_                      Exception::Raiser() ^
#endif

/// Declares methods required for every subclass of honey::Exception.
#define EXCEPTION(Class)                                                        \
    typedef SharedPtr<Class> Ptr;                                               \
    typedef SharedPtr<const Class> ConstPtr;                                    \
                                                                                \
    virtual Exception::Ptr clone() const    { return new Class(*this); }        \
    virtual String typeName() const         { return typeid(*this).name(); }    \
    virtual void raise() const              { throw *this; }                    \


/// Base exception class.  Exceptions inherited from this class provide debug info and can be thrown polymorphically (unlike standard c++ exceptions).
/**
  * All exceptions must inherit from this class and `throw_` should be used instead of the `throw` keyword. \n
  * Default constructed exceptions are fast and lean (no overhead).
  *
  * \see throw_, EXCEPTION
  *
  * Example:
  *
  *     struct MyExA : Exception    { EXCEPTION(MyExA) };
  *     struct MyExB : MyExA        { EXCEPTION(MyExB) };
  *
  *     {
  *         throw_ MyExB() << "Optional Message";    //throw_ directly
  *
  *         MyExA::Ptr a = new MyExB;
  *         throw_ *a << "Error";                    //throw_ polymorphically from base class, catch (MyExB& e)
  *     }
  */
class Exception : public SharedObj<Exception>, public std::exception
{
public:
    /// Custom error message builder
    struct MsgStream : ostringstream
    {
        MsgStream(Exception& e)                                             : e(&e) {}
        MsgStream(MsgStream&& rhs)                                          : ostringstream(std::move(rhs)), e(rhs.e) { rhs.e = nullptr; }
        Exception* e;
    };

    /// Info about source where exception was thrown
    struct Source
    {
        Source()                                                            : func(nullptr), file(nullptr), line(0) {}
        Source(const char* func, const char* file, int line)                : func(func), file(file), line(line) {}
        Exception& operator<<(Exception& e)                                 { e._source = *this; return e; }
        Exception&& operator<<(Exception&& e)                               { e._source = *this; return move(e); }

        friend ostream& operator<<(ostream& os, const Source& source)
        {
            return os << c_str(source.func) << ":" << c_str(source.file) << ":" << source.line;
        }
        
        const char* func;
        const char* file;
        int line;
    };

    /// Helper to raise an exception after the right side of ^ has been evaluated
    struct Raiser
    {
        void operator^(const Exception& e)                                  { e.raise(); }
        void operator^(const ostream& os)                                   { auto& ms = static_cast<const MsgStream&>(os); assert(ms.e); ms.e->_message += ms.str(); ms.e->raise(); }
    };

    Exception() = default;
    Exception(const Exception& rhs)                                         { operator=(rhs); }

    Exception& operator=(const Exception& rhs)
    {
        _source = rhs._source;
        _message = rhs._message;
        _what = nullptr;
        return *this;
    }
    
    EXCEPTION(Exception)

    /// Get info about source where exception was thrown
    const Source& source() const                                            { return _source; }
    /// Get custom error message.  The error message can be appended to using global operator<<(Exception, String)
    const String& message() const                                           { return _message; }
    
    /// Get full diagnostic message
    const char* what() const throw()                                        { cacheWhat(); return _what->c_str(); }

    /// Create a clone of the current exception caught with (...)
    static Ptr current();

    /// Append custom error message
    template<class T>
    MsgStream operator<<(T&& val)                                           { return forward<MsgStream>(MsgStream(*this) << std::forward<T>(val)); }
    
    friend ostream& operator<<(ostream& os, const Exception& e)             { return os << e.what(); }
    
protected:
    /// Create what message.  Called only on demand and result is cached.
    virtual String createWhat() const
    {
        return source().func ?
            sout() << message() << " (exception: " << typeName() << "; " << source() << ")" :
            sout() << message();
    }

private:
    void cacheWhat() const                                                  { if (!_what) _what = honey::make_unique<std::string>(createWhat()); }

    Source _source;
    String _message;
    mutable UniquePtr<std::string> _what;
};

/// Exception util
namespace exception
{
    /// Wrapper around std exception to allow for polymorphic throw
    template<class T>
    struct Std : Exception
    {
        typedef SharedPtr<Std> Ptr;
        typedef SharedPtr<const Std> ConstPtr;
        
        Std(const T& e)                         : _e(e) { *this << _e.what(); }
        
        virtual Exception::Ptr clone() const    { return new Std(_e); }
        virtual String typeName() const         { return typeid(_e).name(); }
        virtual void raise() const              { throw _e; }

    private:
        T _e;
    };

    template<class T>
    typename Std<T>::Ptr createStd(const T& e)  { return new Std<T>(e); }

    struct Unknown : Exception                  { EXCEPTION(Unknown) };
}

inline Exception::Ptr Exception::current()
{
    //std exceptions are not dynamic so there's no way to clone them trivially
    try { throw; }
    catch (Exception& e)                { return e.clone(); }
    catch (std::domain_error& e)        { return honey::exception::createStd(e); }
    catch (std::invalid_argument& e)    { return honey::exception::createStd(e); }
    catch (std::length_error& e)        { return honey::exception::createStd(e); }
    catch (std::out_of_range& e)        { return honey::exception::createStd(e); }
    catch (std::logic_error& e)         { return honey::exception::createStd(e); }
    catch (std::range_error& e)         { return honey::exception::createStd(e); }
    catch (std::overflow_error& e)      { return honey::exception::createStd(e); }
    catch (std::underflow_error& e)     { return honey::exception::createStd(e); }
    catch (std::ios_base::failure& e)   { return honey::exception::createStd(e); }
    catch (std::runtime_error& e)       { return honey::exception::createStd(e); }
    catch (std::bad_alloc& e)           { return honey::exception::createStd(e); }
    catch (std::bad_cast& e)            { return honey::exception::createStd(e); }
    catch (std::bad_typeid& e)          { return honey::exception::createStd(e); }
    catch (std::bad_exception& e)       { return honey::exception::createStd(e); }
    catch (std::exception& e)           { return honey::exception::createStd(e); }
    catch (...)                         { return new honey::exception::Unknown(); }
}

namespace debug
{
    /// Thrown on debug assert() failure
    struct AssertionFailure : Exception { EXCEPTION(AssertionFailure) };
}

}

