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
        void operator^(const Exception& e)                                  { e.raise(); }
        void operator^(const ostream& os)                                   { auto& ms = static_cast<const MsgStream&>(os); assert(ms.e); ms.e->_message += ms.str(); ms.e->raise(); }
    };

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
    
    EXCEPTION(Exception)

    /// Get info about source where exception was thrown
    const Source& source() const                                            { return _source; }
    /// Get custom error message.  The error message can be appended to using global operator<<(Exception, String)
    const String& message() const                                           { return _message; }
    
    /// Get full diagnostic message
    const String& what_() const                                             { cacheWhat(); return *_what; }
    /// Get full diagnostic message
    const char* what() const throw()                                        { what_(); return _what_u8->c_str(); }

    /// Create a clone of the current exception caught with (...)
    static Ptr current();

    /// Append custom error message
    template<class T>
    MsgStream operator<<(T&& val)                                           { return forward<MsgStream>(MsgStream(*this) << std::forward<T>(val)); }
    
    friend ostream& operator<<(ostream& os, const Exception& e)             { return os << e.what_(); }
    
protected:
    /// Create what message.  Called only on demand and result is cached.
    virtual String createWhat() const
    {
        return sout()
            << "Exception:  "   << typeName()       << endl
            << "Message:    "   << message()        << endl
            << source();
    }

private:
    void cacheWhat() const                                                  { if (_what) return; _what = new String(createWhat()); _what_u8 = new std::string(_what->u8()); }

    Source _source;
    String _message;
    mutable UniquePtr<String> _what;
    mutable UniquePtr<std::string> _what_u8;
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

        virtual String createWhat() const
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
    typename Std<T>::Ptr createStd(const T& e)  { return new Std<T>(e); }

    struct Unknown : Exception                  { EXCEPTION(Unknown) };
} }
/** \endcond */

inline Exception::Ptr Exception::current()
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
    catch(...)                          { return new Unknown(); }
}

namespace debug
{
    /// Thrown on debug assert() failure
    struct AssertionFailure : Exception { EXCEPTION(AssertionFailure) };
}

}

