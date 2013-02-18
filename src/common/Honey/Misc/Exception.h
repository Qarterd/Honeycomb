// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/Stream.h"
#include "Honey/Memory/SharedPtr.h"
#include "Honey/Memory/UniquePtr.h"

namespace honey
{

/// Use in place of `throw` keyword to throw a honey::Exception object polymorphically and provide debug info
#ifndef FINAL
    #define throw_                      Exception::Raiser() ^ Exception::Source(__FUNC__, __FILE__, __LINE__) << 
#else
    #define throw_                      Exception::Raiser() ^
#endif

/// Declares methods required for every subclass of honey::Exception.
#define EXCEPTION(Class)                                                    \
    typedef SharedPtr<Class> Ptr;                                           \
    typedef SharedPtr<const Class> ConstPtr;                                \
                                                                            \
    virtual Class& clone() const        { return *new Class(*this); }       \
    virtual String typeName() const     { return typeid(*this).name(); }    \
    virtual void raise() const          { throw *this; }                    \


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
  *         MyExA::Ptr a = new MyExB;
  *
  *         throw_ *a << "Error";                    //throw_ polymorphically from base class, catch (MyExB& e)
  *     }
  */
class Exception : public SharedObj<Exception>, public std::exception
{
public:
    Exception()                                                             {}
    Exception(const Exception& rhs)                                         { operator=(rhs); }

    Exception& operator=(const Exception& rhs)
    {
        _source = rhs._source;
        _message = rhs._message;
        _what = nullptr;
        _what_u8 = nullptr;
        return *this;
    }

    /// Info about source where exception was thrown
    struct Source
    {
        Source()                                                            : func(nullptr), file(nullptr), line(0) {}
        Source(const char* func, const char* file, int line)                : func(func), file(file), line(line) {}
        template<class E> E& operator<<(E&& e)                              { e._source = *this; return e; }

        friend ostream& operator<<(ostream& os, const Source& source)
        {
            return os   << "Function:   " << c_str(source.func) << endl
                        << "File:       " << c_str(source.file) << ":" << source.line << endl;
        }

        const char* func;
        const char* file;
        int line;
    };

    /// Helper to raise an exception after the right side of ^ has been evaluated
    struct Raiser
    {
        template<class E> void operator^(const E& e)                        { e.raise(); }
    };

    EXCEPTION(Exception)

    /// Get info about source where exception was thrown
    const Source& source() const                                            { return _source; }
    /// Get custom error message.  The error message can be appended to using global operator<<(Exception, String)
    const String& message() const                                           { return _message; }
    
    /// Get full diagnostic message
    const String& what_() const                                             { const_cast<Exception*>(this)->cacheWhat(); return *_what; }
    /// Get full diagnostic message
    const char* what() const throw()                                        { what_(); return _what_u8->c_str(); }

    /// Create a clone of the current exception caught with (...)
    static Exception& current();

    friend ostream& operator<<(ostream& os, const Exception& e)   { return os << e.what_(); }

protected:

    /// Create what message.  Called only on demand and result is cached.
    virtual String createWhat()
    {
        return sout()
            << "Exception:  "   << typeName()       << endl
            << "Message:    "   << message()        << endl
            << source();
    }

private:
    /// operator<<(Exception, String)   Append custom error message to exception
    template<class E>
    friend typename std::enable_if<mt::is_base_of<Exception, E>::value, E>::type&
        operator<<(E&& e, const String& message)                            { e._message += message; return e; }

    void cacheWhat()
    {
        if (_what) return;
        _what = new String(createWhat());
        _what_u8 = new std::string(_what->u8());
    }

    Source _source;
    String _message;
    UniquePtr<String> _what;
    UniquePtr<std::string> _what_u8;
};

/** \cond */
namespace exception { namespace priv
{
    /// Wrapper around std exception to make dynamic
    template<class T>
    struct Std : Exception
    {
        EXCEPTION(Std)
        Std(const T& e)                         : _e(e) {}

        virtual String createWhat()
        {
            return sout()
                << "Exception:  "   << typeid(_e).name()        << endl
                << "Message:    "   << message() << _e.what()   << endl
                << source();
        }

    private:
        T _e;
    };

    template<class T>
    Std<T>& createStd(const T& e)               { return *new Std<T>(e); }

    struct Unknown : Exception                  { EXCEPTION(Unknown) };
} }
/** \endcond */

inline Exception& Exception::current()
{
    using namespace honey::exception::priv;
    //std exceptions are not dynamic so there's no way to clone them trivially
    try { throw; }
    catch(Exception& e)                 { return e.clone(); }
    catch(std::domain_error& e)         { return createStd(e); }
    catch(std::invalid_argument& e)     { return createStd(e); }
    catch(std::length_error& e)         { return createStd(e); }
    catch(std::out_of_range& e)         { return createStd(e); }
    catch(std::logic_error& e)          { return createStd(e); }
    catch(std::range_error& e)          { return createStd(e); }
    catch(std::overflow_error& e)       { return createStd(e); }
    catch(std::underflow_error& e)      { return createStd(e); }
    catch(std::ios_base::failure& e)    { return createStd(e); }
    catch(std::runtime_error& e)        { return createStd(e); }
    catch(std::bad_alloc& e)            { return createStd(e); }
    catch(std::bad_cast& e)             { return createStd(e); }
    catch(std::bad_typeid& e)           { return createStd(e); }
    catch(std::bad_exception& e)        { return createStd(e); }
    catch(std::exception& e)            { return createStd(e); }
    catch(...)                          { return *new Unknown(); }
}

namespace debug
{
    /// Thrown on debug assert() failure
    struct AssertionFailure : Exception { EXCEPTION(AssertionFailure) };
}

}

