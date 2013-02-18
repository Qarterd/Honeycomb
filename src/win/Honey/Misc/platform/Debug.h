// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma once

/** \cond */
namespace honey
{

class String;

namespace platform
{

class Debug
{
public:
    static void print(const String& str);
    static void assertPrint(const String& expr, const String& file, int line, const String& msg);
};

} }
/** \endcond */
