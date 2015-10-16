// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Net/Stream.h"
#include "Honey/Net/platform/Socket.h"

namespace honey
{
/// Net util
namespace net
{

/// Socket util
namespace socket
{
}

/// Socket class
class Socket : platform::Socket, mt::NoCopy
{
    typedef platform::Socket Super;
    friend class platform::Socket;

public:

};

} }
