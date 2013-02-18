// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

//====================================================
// Standard C lib
//====================================================
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

//====================================================
// Standard C++ lib
//====================================================
#include <algorithm>
#include <array>
#include <cctype>
#include <complex>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//====================================================
// Honeycomb
//====================================================
namespace honey
{

using std::array;
using std::begin;
using std::bind;
using std::complex;
using std::cref;
using std::declval;
using std::deque;
using std::end;
using std::find_if;
using std::forward;
using std::forward_as_tuple;
using std::function;
using std::get;
using std::ignore;
using std::initializer_list;
using std::ios_base;
using std::istream;
using std::istringstream;
using std::list;
using std::make_pair;
using std::make_tuple;
using std::max;
using std::min;
using std::move;
using std::multimap;
using std::multiset;
using std::next;
using std::nullptr_t;
using std::ostream;
using std::ostringstream;
using std::pair;
using namespace std::placeholders;
using std::prev;
using std::ref;
using std::set;
using std::tie;
using std::tuple;
using std::tuple_element;
using std::tuple_size;
using std::unordered_map;
using std::unordered_multimap;
using std::unordered_multiset;
using std::unordered_set;
using std::vector;

/// Options for platform endian
#define ENDIAN_LITTLE   0
#define ENDIAN_BIG      1

}

#include "Honey/Core/platform/Core.h"
#include "Honey/Memory/Allocator.h"

