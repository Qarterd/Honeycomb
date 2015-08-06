// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"
#include "Honey/Misc/platform/Debug.h"

namespace honey
{

class String;

/**
  * \defgroup Debug     Debug Mode Functions
  */
/// @{

/// \ref Debug
namespace debug
{
    /// \addtogroup Debug
    /// @{

    /// Forwards to assert_\#args. See assert_1(), assert_2().
    #define assert(...)                                 EVAL(TOKCAT(assert_, NUMARGS(__VA_ARGS__))(__VA_ARGS__))
    /// Forwards to verify_\#args. See verify_1(), verify_2().
    #define verify(...)                                 EVAL(TOKCAT(verify_, NUMARGS(__VA_ARGS__))(__VA_ARGS__))

    #ifndef FINAL
        /// Evaluate expression in debug mode only, does nothing in final mode.
        #define debug_if(...)                           __VA_ARGS__
        /// Print string to debug output window.  Does nothing in final mode.
        #define debug_print(...)                        { honey::debug::platform::print(__VA_ARGS__); }
        /// Cause debugger to break. Debugger must have exception breakpoints enabled.
        #define debug_break(msg)                        { try { error_(msg); } catch (Exception&) {} }
        /// Assert that an expression is true, otherwise throws AssertionFailure with the expression. Does nothing in final mode.
        #define assert_1(expr)                          assert_2(expr, "")
        /// Assert with extra message to be displayed on failure
        #define assert_2(expr, msg)                     if (!(expr)) { honey::debug::platform::assertFail(#expr, __FUNC__, __FILE__, __LINE__, (msg)); }
        /// Similar to assert() but evaluates the expression and throws an error even in final mode
        #define verify_1(expr)                          assert_1(expr)
        /// Verify with extra message to be displayed on failure.  Message ignored in final mode.
        #define verify_2(expr, msg)                     assert_2(expr, msg)
        /// Throw AssertionFailure with a message.  Message ignored in final mode.
        #define error_(msg)                             assert_2(false, msg)
        /// Flag to check if debug mode is enabled at compile-time/runtime
        static const bool enabled                       = true;
    #else
        #define debug_if(...)
        #define debug_print(...) {}
        #define debug_break(msg) {}
        #define assert_1(expr) {}
        #define assert_2(expr, msg) {}
        #define verify_1(expr)                          verify_2(expr, "")
        #define verify_2(expr, msg)                     if (!(expr)) { honey::debug::platform::assertFail("", "", "", 0, ""); }
        #define error_(msg)                             verify_2(false, msg)
        static const bool enabled                       = false;
    #endif
    
    /// @}
}

/// @}

}
