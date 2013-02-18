// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Core/Core.h"

namespace honey
{

/// \defgroup Preprocessor  Preprocessor Macros
/// @{

/// Evaluate expression
#define EVAL(...)                                           __VA_ARGS__

/// Convert token to string
#define STRINGIFY(s)                                        STRINGIFY_(s)

/// Concatenate tokens together to form a new symbol
#define TOKCAT(a, b)                                        TOKCAT_(a, b)

/// Symbol evaluates to nothing
#define EMPTY
/// Function evaluates to nothing
#define EMPTYFUNC(...)

/// Remove brackets around parameter
#define UNBRACKET(...)                                      IFEQUAL(_UNBRACKET_TEST __VA_ARGS__, 1, EVAL __VA_ARGS__, __VA_ARGS__)

/// Get number of arguments in list.  Supports up to 10 args (limited because of high impact on compile-time).
#define NUMARGS(...)                                        IFEMPTY(0, _NUMARGS(__VA_ARGS__), __VA_ARGS__)

/// Use to avoid macro interpreting a comma as an argument delimiter
#define COMMA                                               ,
/// If Num is not CompareNum, then this will evaluate to a comma.  Useful for iteration over function parameters.
#define COMMA_IFNOT(Num, CompareNum)                        IFEQUAL(Num, CompareNum, EMPTY, COMMA)

/// Evaluate true or false depending on whether argument list is empty
#define IFEMPTY(True, False, ...)                           TOKCAT(_IFEMPTY_CASE_, _ISEMPTY(__VA_ARGS__))(EVAL(True), EVAL(False))

/// Evaluate True if Num == CompareNum, otherwise eval False.
#define IFEQUAL(Num, CompareNum, True, False)               IFEMPTY(EVAL(True), EVAL(False), TOKCAT(_ITERATE_, TOKCAT(Num, _##CompareNum)))

/// Get number of elements in tuple
#define TUPLE_SIZE(t)                                       EVAL(NUMARGS t)
/// Get element in tuple. A tuple is a bracketed parameter list: (e0,e1,e2...).  Supports up to 5 elements.
#define TUPLE_ELEM(t, i)                                    TOKCAT(_TUPLE_ELEM_,TUPLE_SIZE(t))_ ## i t

/// \name ITERATE
/// @{

/// Max number of iterations supported
#define ITERATE_MAX                                         20

/// Iterate calling Func(It, Args...) over range [Min, Max], where Min >= 0 and Max <= ITERATE_MAX.  `It` is the current iteration number.
/**
  * If Min > Max then iterate will do nothing, eg. Iterate(1,0,FUNC)
  */
#define ITERATE(Min, Max, Func)                             IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 0,,,,,)             , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE1(Min, Max, Func, a1)                        IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 1,a1,,,,)           , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE2(Min, Max, Func, a1,a2)                     IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 2,a1,a2,,,)         , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE3(Min, Max, Func, a1,a2,a3)                  IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 3,a1,a2,a3,,)       , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE4(Min, Max, Func, a1,a2,a3,a4)               IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 4,a1,a2,a3,a4,)     , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE5(Min, Max, Func, a1,a2,a3,a4,a5)            IFEMPTY(, TOKCAT(_ITERATE_, Min)(Max, Func, 5,a1,a2,a3,a4,a5)   , TOKCAT(_ITERATE_INIT_##Min##_, Max))
/// @}

/**
  * \name ITERATE_
  * Clone of ITERATE functions to allow for recursion.
  *
  * Macros don't support recursion, ITERATE can't called from inside ITERATE.
  * Instead, change the inner recursive call to ITERATE_ or ITERATE__.
  */
/// @{
#define ITERATE_(Min, Max, Func)                            IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 0,,,,,)            , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE1_(Min, Max, Func, a1)                       IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 1,a1,,,,)          , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE2_(Min, Max, Func, a1,a2)                    IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 2,a1,a2,,,)        , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE3_(Min, Max, Func, a1,a2,a3)                 IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 3,a1,a2,a3,,)      , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE4_(Min, Max, Func, a1,a2,a3,a4)              IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 4,a1,a2,a3,a4,)    , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE5_(Min, Max, Func, a1,a2,a3,a4,a5)           IFEMPTY(, TOKCAT(_ITERATE__, Min)(Max, Func, 5,a1,a2,a3,a4,a5)  , TOKCAT(_ITERATE_INIT_##Min##_, Max))

#define ITERATE__(Min, Max, Func)                           IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 0,,,,,)           , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE1__(Min, Max, Func, a1)                      IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 1,a1,,,,)         , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE2__(Min, Max, Func, a1,a2)                   IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 2,a1,a2,,,)       , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE3__(Min, Max, Func, a1,a2,a3)                IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 3,a1,a2,a3,,)     , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE4__(Min, Max, Func, a1,a2,a3,a4)             IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 4,a1,a2,a3,a4,)   , TOKCAT(_ITERATE_INIT_##Min##_, Max))
#define ITERATE5__(Min, Max, Func, a1,a2,a3,a4,a5)          IFEMPTY(, TOKCAT(_ITERATE___, Min)(Max, Func, 5,a1,a2,a3,a4,a5) , TOKCAT(_ITERATE_INIT_##Min##_, Max))
/// @}

/// Add two values, result must be in range [0, ITERATE_MAX]
#define PP_ADD(Lhs, Rhs)                                    _ADD_0(Lhs, Rhs)
/// Subtract two values in range [0, ITERATE_MAX]
#define PP_SUB(Lhs, Rhs)                                    _SUBTRACT_0(Lhs, Rhs)


/// @}

//====================================================
// Private
//====================================================
#define TOKCAT_(a, b)                                       a##b
#define STRINGIFY_(s)                                       #s
#define _TOKCAT_2(a1, a2)                                   a1 ## a2
#define _TOKCAT_3(a1, a2, a3)                               a1 ## a2 ## a3
#define _TOKCAT_4(a1, a2, a3, a4)                           a1 ## a2 ## a3 ## a4
#define _TOKCAT_5(a1, a2, a3, a4, a5)                       a1 ## a2 ## a3 ## a4 ## a5
#define _TOKCAT(...)                                        EVAL(TOKCAT(_TOKCAT_, _NUMARGS(__VA_ARGS__))(__VA_ARGS__))

#define _UNBRACKET_TEST(...)                                1

#define _NUMARGS(...)                                                               \
    EVAL(_NUMARGS_FUNC(__VA_ARGS__,                                                 \
     10,  9,  8,  7,  6,  5,  4,  3,  2,  1                                         \
    ))                                                                              \

#define _NUMARGS_FUNC(                                                              \
    _00,_01,_02,_03,_04,_05,_06,_07,_08,_09,                                        \
    N,...) N                                                                        \

/// Check if argument has a comma.  Returns 1 if true, 0 if false
#define _HASCOMMA(...)                                                              \
    EVAL(_NUMARGS_FUNC(__VA_ARGS__,                                                 \
      1,  1,  1,  1,  1,  1,  1,  1,  1,  0,                                        \
    ))                                                                              \

/// Returns 1 if argument list is empty, 0 otherwise
#define _ISEMPTY(...)                                                                                   \
        _HASCOMMA(  _TOKCAT( _ISEMPTY_CASE_,    _HASCOMMA(__VA_ARGS__),                                 \
                                                _HASCOMMA(_ISEMPTY_BRACKET_TEST __VA_ARGS__),           \
                                                _HASCOMMA(__VA_ARGS__ (EMPTY)),                         \
                                                _HASCOMMA(_ISEMPTY_BRACKET_TEST __VA_ARGS__ (EMPTY))    \
                    )                                                                                   \
        )                                                                                               \

#define _ISEMPTY_BRACKET_TEST(...)                          ,
#define _ISEMPTY_CASE_0001                                  ,
#define _IFEMPTY_CASE_0(True, False)                        False
#define _IFEMPTY_CASE_1(True, False)                        True

#define _TUPLE_ELEM_1_0(_0)                                 _0
#define _TUPLE_ELEM_2_0(_0,_1)                              _0
#define _TUPLE_ELEM_2_1(_0,_1)                              _1
#define _TUPLE_ELEM_3_0(_0,_1,_2)                           _0
#define _TUPLE_ELEM_3_1(_0,_1,_2)                           _1
#define _TUPLE_ELEM_3_2(_0,_1,_2)                           _2
#define _TUPLE_ELEM_4_0(_0,_1,_2,_3)                        _0
#define _TUPLE_ELEM_4_1(_0,_1,_2,_3)                        _1
#define _TUPLE_ELEM_4_2(_0,_1,_2,_3)                        _2
#define _TUPLE_ELEM_4_3(_0,_1,_2,_3)                        _3
#define _TUPLE_ELEM_5_0(_0,_1,_2,_3,_4)                     _0
#define _TUPLE_ELEM_5_1(_0,_1,_2,_3,_4)                     _1
#define _TUPLE_ELEM_5_2(_0,_1,_2,_3,_4)                     _2
#define _TUPLE_ELEM_5_3(_0,_1,_2,_3,_4)                     _3
#define _TUPLE_ELEM_5_4(_0,_1,_2,_3,_4)                     _4

/// Prevent iteration in common cases where user calls with min > max
#define _ITERATE_INIT_1_0
#define _ITERATE_INIT_2_0
#define _ITERATE_INIT_2_1

/// Iteration stop
#define _ITERATE_0_0
#define _ITERATE_1_1
#define _ITERATE_2_2
#define _ITERATE_3_3
#define _ITERATE_4_4
#define _ITERATE_5_5
#define _ITERATE_6_6
#define _ITERATE_7_7
#define _ITERATE_8_8
#define _ITERATE_9_9
#define _ITERATE_10_10
#define _ITERATE_11_11
#define _ITERATE_12_12
#define _ITERATE_13_13
#define _ITERATE_14_14
#define _ITERATE_15_15
#define _ITERATE_16_16
#define _ITERATE_17_17
#define _ITERATE_18_18
#define _ITERATE_19_19
#define _ITERATE_20_20


#define _ITERATE_FUNC_0(Func, It, a1,a2,a3,a4,a5 ) Func(It)
#define _ITERATE_FUNC_1(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1)
#define _ITERATE_FUNC_2(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2)
#define _ITERATE_FUNC_3(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3)
#define _ITERATE_FUNC_4(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4)
#define _ITERATE_FUNC_5(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4,a5)

#define _ITERATE_0(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 0, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_1(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_0_, Max))
#define _ITERATE_1(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 1, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_2(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_1_, Max))
#define _ITERATE_2(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 2, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_3(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_2_, Max))
#define _ITERATE_3(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 3, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_4(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_3_, Max))
#define _ITERATE_4(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 4, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_5(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_4_, Max))
#define _ITERATE_5(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 5, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_6(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_5_, Max))
#define _ITERATE_6(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 6, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_7(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_6_, Max))
#define _ITERATE_7(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 7, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_8(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_7_, Max))
#define _ITERATE_8(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 8, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_9(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_8_, Max))
#define _ITERATE_9(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 9, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_10(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_9_, Max))
#define _ITERATE_10(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 10, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_11(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_10_, Max))
#define _ITERATE_11(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 11, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_12(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_11_, Max))
#define _ITERATE_12(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 12, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_13(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_12_, Max))
#define _ITERATE_13(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 13, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_14(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_13_, Max))
#define _ITERATE_14(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 14, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_15(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_14_, Max))
#define _ITERATE_15(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 15, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_16(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_15_, Max))
#define _ITERATE_16(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 16, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_17(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_16_, Max))
#define _ITERATE_17(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 17, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_18(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_17_, Max))
#define _ITERATE_18(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 18, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_19(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_18_, Max))
#define _ITERATE_19(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 19, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_20(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_19_, Max))
#define _ITERATE_20(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC_, ac)(Func, 20, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE_21(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_20_, Max))


#define _ITERATE_FUNC__0(Func, It, a1,a2,a3,a4,a5 ) Func(It)
#define _ITERATE_FUNC__1(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1)
#define _ITERATE_FUNC__2(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2)
#define _ITERATE_FUNC__3(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3)
#define _ITERATE_FUNC__4(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4)
#define _ITERATE_FUNC__5(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4,a5)

#define _ITERATE__0(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 0, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__1(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_0_, Max))
#define _ITERATE__1(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 1, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__2(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_1_, Max))
#define _ITERATE__2(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 2, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__3(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_2_, Max))
#define _ITERATE__3(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 3, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__4(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_3_, Max))
#define _ITERATE__4(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 4, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__5(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_4_, Max))
#define _ITERATE__5(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 5, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__6(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_5_, Max))
#define _ITERATE__6(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 6, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__7(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_6_, Max))
#define _ITERATE__7(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 7, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__8(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_7_, Max))
#define _ITERATE__8(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 8, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__9(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_8_, Max))
#define _ITERATE__9(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 9, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__10(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_9_, Max))
#define _ITERATE__10(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 10, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__11(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_10_, Max))
#define _ITERATE__11(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 11, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__12(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_11_, Max))
#define _ITERATE__12(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 12, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__13(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_12_, Max))
#define _ITERATE__13(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 13, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__14(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_13_, Max))
#define _ITERATE__14(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 14, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__15(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_14_, Max))
#define _ITERATE__15(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 15, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__16(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_15_, Max))
#define _ITERATE__16(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 16, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__17(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_16_, Max))
#define _ITERATE__17(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 17, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__18(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_17_, Max))
#define _ITERATE__18(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 18, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__19(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_18_, Max))
#define _ITERATE__19(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 19, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__20(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_19_, Max))
#define _ITERATE__20(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC__, ac)(Func, 20, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE__21(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_20_, Max))


#define _ITERATE_FUNC___0(Func, It, a1,a2,a3,a4,a5 ) Func(It)
#define _ITERATE_FUNC___1(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1)
#define _ITERATE_FUNC___2(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2)
#define _ITERATE_FUNC___3(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3)
#define _ITERATE_FUNC___4(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4)
#define _ITERATE_FUNC___5(Func, It, a1,a2,a3,a4,a5 ) Func(It,a1,a2,a3,a4,a5)

#define _ITERATE___0(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 0, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___1(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_0_, Max))
#define _ITERATE___1(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 1, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___2(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_1_, Max))
#define _ITERATE___2(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 2, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___3(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_2_, Max))
#define _ITERATE___3(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 3, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___4(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_3_, Max))
#define _ITERATE___4(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 4, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___5(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_4_, Max))
#define _ITERATE___5(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 5, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___6(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_5_, Max))
#define _ITERATE___6(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 6, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___7(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_6_, Max))
#define _ITERATE___7(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 7, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___8(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_7_, Max))
#define _ITERATE___8(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 8, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___9(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_8_, Max))
#define _ITERATE___9(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 9, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___10(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_9_, Max))
#define _ITERATE___10(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 10, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___11(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_10_, Max))
#define _ITERATE___11(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 11, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___12(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_11_, Max))
#define _ITERATE___12(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 12, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___13(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_12_, Max))
#define _ITERATE___13(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 13, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___14(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_13_, Max))
#define _ITERATE___14(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 14, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___15(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_14_, Max))
#define _ITERATE___15(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 15, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___16(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_15_, Max))
#define _ITERATE___16(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 16, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___17(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_16_, Max))
#define _ITERATE___17(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 17, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___18(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_17_, Max))
#define _ITERATE___18(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 18, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___19(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_18_, Max))
#define _ITERATE___19(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 19, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___20(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_19_, Max))
#define _ITERATE___20(Max, Func, ac,a1,a2,a3,a4,a5 ) TOKCAT(_ITERATE_FUNC___, ac)(Func, 20, a1,a2,a3,a4,a5 ) IFEMPTY(, _ITERATE___21(Max, Func, ac,a1,a2,a3,a4,a5 ), TOKCAT(_ITERATE_20_, Max))

#define _INC_0 1
#define _INC_1 2
#define _INC_2 3
#define _INC_3 4
#define _INC_4 5
#define _INC_5 6
#define _INC_6 7
#define _INC_7 8
#define _INC_8 9
#define _INC_9 10
#define _INC_10 11
#define _INC_11 12
#define _INC_12 13
#define _INC_13 14
#define _INC_14 15
#define _INC_15 16
#define _INC_16 17
#define _INC_17 18
#define _INC_18 19
#define _INC_19 20

#define _ADD_0(Lhs, Rhs) IFEQUAL(Rhs, 0, Lhs, _ADD_1(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_1(Lhs, Rhs) IFEQUAL(Rhs, 1, Lhs, _ADD_2(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_2(Lhs, Rhs) IFEQUAL(Rhs, 2, Lhs, _ADD_3(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_3(Lhs, Rhs) IFEQUAL(Rhs, 3, Lhs, _ADD_4(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_4(Lhs, Rhs) IFEQUAL(Rhs, 4, Lhs, _ADD_5(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_5(Lhs, Rhs) IFEQUAL(Rhs, 5, Lhs, _ADD_6(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_6(Lhs, Rhs) IFEQUAL(Rhs, 6, Lhs, _ADD_7(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_7(Lhs, Rhs) IFEQUAL(Rhs, 7, Lhs, _ADD_8(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_8(Lhs, Rhs) IFEQUAL(Rhs, 8, Lhs, _ADD_9(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_9(Lhs, Rhs) IFEQUAL(Rhs, 9, Lhs, _ADD_10(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_10(Lhs, Rhs) IFEQUAL(Rhs, 10, Lhs, _ADD_11(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_11(Lhs, Rhs) IFEQUAL(Rhs, 11, Lhs, _ADD_12(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_12(Lhs, Rhs) IFEQUAL(Rhs, 12, Lhs, _ADD_13(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_13(Lhs, Rhs) IFEQUAL(Rhs, 13, Lhs, _ADD_14(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_14(Lhs, Rhs) IFEQUAL(Rhs, 14, Lhs, _ADD_15(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_15(Lhs, Rhs) IFEQUAL(Rhs, 15, Lhs, _ADD_16(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_16(Lhs, Rhs) IFEQUAL(Rhs, 16, Lhs, _ADD_17(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_17(Lhs, Rhs) IFEQUAL(Rhs, 17, Lhs, _ADD_18(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_18(Lhs, Rhs) IFEQUAL(Rhs, 18, Lhs, _ADD_19(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_19(Lhs, Rhs) IFEQUAL(Rhs, 19, Lhs, _ADD_20(TOKCAT(_INC_,Lhs),Rhs))
#define _ADD_20(Lhs, Rhs) IFEQUAL(Rhs, 20, Lhs, _ADD_21(TOKCAT(_INC_,Lhs),Rhs))

#define _DEC_1 0
#define _DEC_2 1
#define _DEC_3 2
#define _DEC_4 3
#define _DEC_5 4
#define _DEC_6 5
#define _DEC_7 6
#define _DEC_8 7
#define _DEC_9 8
#define _DEC_10 9
#define _DEC_11 10
#define _DEC_12 11
#define _DEC_13 12
#define _DEC_14 13
#define _DEC_15 14
#define _DEC_16 15
#define _DEC_17 16
#define _DEC_18 17
#define _DEC_19 18
#define _DEC_20 19

#define _SUBTRACT_0(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 0, _SUBTRACT_1(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_1(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 1, _SUBTRACT_2(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_2(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 2, _SUBTRACT_3(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_3(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 3, _SUBTRACT_4(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_4(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 4, _SUBTRACT_5(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_5(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 5, _SUBTRACT_6(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_6(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 6, _SUBTRACT_7(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_7(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 7, _SUBTRACT_8(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_8(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 8, _SUBTRACT_9(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_9(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 9, _SUBTRACT_10(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_10(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 10, _SUBTRACT_11(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_11(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 11, _SUBTRACT_12(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_12(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 12, _SUBTRACT_13(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_13(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 13, _SUBTRACT_14(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_14(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 14, _SUBTRACT_15(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_15(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 15, _SUBTRACT_16(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_16(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 16, _SUBTRACT_17(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_17(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 17, _SUBTRACT_18(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_18(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 18, _SUBTRACT_19(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_19(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 19, _SUBTRACT_20(TOKCAT(_DEC_,Lhs),Rhs))
#define _SUBTRACT_20(Lhs, Rhs) IFEQUAL(Lhs, Rhs, 20, _SUBTRACT_21(TOKCAT(_DEC_,Lhs),Rhs))


}