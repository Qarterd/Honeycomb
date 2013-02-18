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
        /// Assert that an expression is true, otherwise throws AssertionFailure with the expression. Does nothing in final mode.
        #define assert_1(Expr)                          assert_2(Expr, "")
        /// Assert with extra message to be displayed on failure
        #define assert_2(Expr, Msg)                     if (!(Expr)) { honey::debug::platform::assertFail(#Expr, __FUNC__, __FILE__, __LINE__, (Msg)); }
        /// Similar to assert() but evaluates the expression and throws an error even in final mode
        #define verify_1(Expr)                          assert_1(Expr)
        /// Verify with extra message to be displayed on failure.  Message ignored in final mode.
        #define verify_2(Expr, Msg)                     assert_2(Expr, Msg)
        /// Throw AssertionFailure with a message.  Message ignored in final mode.
        #define error(Msg)                              assert_2(false, Msg)
    #else
        #define debug_if(...)
        #define debug_print(...) {}
        #define assert_1(Expr) {}
        #define assert_2(Expr, Msg) {}
        #define verify_1(Expr)                          verify_2(Expr, "")
        #define verify_2(Expr, Msg)                     if (!(Expr)) { honey::debug::platform::assertFail("", "", "", 0, ""); }
        #define error(Msg)                              verify_2(false, Msg)
    #endif
    
    /// @}
}

/// @}

}
