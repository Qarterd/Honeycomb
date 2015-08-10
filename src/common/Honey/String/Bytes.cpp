// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Bytes.h"
#include "Honey/String/Encode.h"

namespace honey
{

ostream& operator<<(ostream& os, const Bytes& val)
{
    const Id& enc = encode::priv::Manip::inst(os).encoding;
    assert(encode::priv::exts().count(enc), sout() << "Encoding not found: " << enc);
    return encode::priv::exts().at(enc)[encode::priv::encode()](os, val);
}

istream& operator>>(istream& is, Bytes& val)
{
    const Id& enc = encode::priv::Manip::inst(is).encoding;
    assert(encode::priv::exts().count(enc), sout() << "Encoding not found: " << enc);
    return encode::priv::exts().at(enc)[encode::priv::decode()](is, val);
}

}
