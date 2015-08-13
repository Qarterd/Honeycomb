// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

class String;

namespace debug { namespace platform
{
    void print(const String& str);
    void print(const char* str);
    
    void assertFail(const char* expr, const char* func, const char* file, int line, const String& msg);
    void assertFail(const char* expr, const char* func, const char* file, int line, const char* msg);
} }

}
/** \endcond */
