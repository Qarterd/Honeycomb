// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/platform/Random.h"
#include <fcntl.h>

/** \cond */
namespace honey { namespace random { namespace platform
{

Bytes deviceEntropy(szt count)
{
    Bytes rand(count);
    //use unbuffered I/O since we are likely reading a small number of bytes
    int src = open("/dev/urandom", O_RDONLY);
    read(src, rand.data(), count);
    close(src);
    return rand;
}

} } }
/** \endcond */