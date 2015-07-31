// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"

namespace honey
{
/// String util
namespace string
{

/// Parse command-line arguments into argc/argv
void parseArgv(const String& str, int& argc, char**& argv);
void deleteArgv(int argc, char**& argv);

} }
