// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Misc/Debug.h"
#include "Honey/Misc/Exception.h"

/** \cond */
namespace honey { namespace platform
{

#ifndef FINAL
    void Debug::print(const String& str)
    {
        OutputDebugStringW(str.toStdWString().c_str());
    }

    void Debug::assertPrint(const String& expr, const String& file, int line, const String& msg)
    {
        #ifdef DEBUG
            String assert = expr + "\nMessage: " + msg;
            _wassert(assert.toStdWString().c_str(), file.toStdWString().c_str(), line);
        #else
            String assert = sout()
                << "Assertion failed!" << endl
                << "File: " << file << endl
                << "Line: " << line << endl
                << "Expression: " << expr << endl
                << "Message: " << msg << endl;
            print(assert);
            Exception::Raiser() ^ Exception::Source("", file.toStdString().c_str(), line) << Exception() << assert;
        #endif
    }
#endif

} }
/** \endcond */


