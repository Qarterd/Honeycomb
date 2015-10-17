// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/ByteStream.h"
#include "Honey/Math/Alge/Alge.h"

namespace honey { namespace net
{

/// automatically resizable buffer class based on ByteStreamBuf
/**
  * Examples:
  *
  * Writing directly from a StreamBuf to a socket:
  *
  *     net::StreamBuf b;
  *     ostream os(&b);
  *     os << "Hello, World!";
  *
  *     szt n = sock.send(b.data()); //try sending some data from input sequence
  *     b.consume(n); //sent data is removed from input sequence
  *
  * Reading from a socket directly into a StreamBuf:
  *
  *     net::StreamBuf b;
  *     ByteBuf buf = b.prepare(512); //reserve 512 bytes in output sequence
  *
  *     szt n = sock.receive(buf); //receive some data into buffer
  *     b.commit(n); //received data is "committed" from output sequence to input sequence
  *
  *     istream is(&b);
  *     String s;
  *     is >> s;
  */
template<class Alloc = std::allocator<byte>>
class StreamBuf_ : public ByteStreamBuf, mt::NoCopy
{
public:
    StreamBuf_(szt maxSize = numeral<szt>().max(), const Alloc& alloc = Alloc()) :
        _maxSize(maxSize),
        _buf(alloc)
    {
        auto pend = Alge::min(_maxSize, buf_delta);
        _buf.resize(Alge::max(pend, 1));
        setg(&_buf[0], &_buf[0], &_buf[0]);
        setp(&_buf[0], &_buf[0] + pend);
    }

    /// Get the size of the input sequence
    szt size() const                        { return pptr() - gptr(); }
    /// Get the max sum of sizes of the input and output sequences
    szt maxSize() const                     { return _maxSize; }
    /// Get the data that represents the input sequence
    ByteBufConst data() const               { return ByteBufConst(gptr(), size()); }

    /// Get a buffer that represents the output sequence with the given size
    /**
      * Ensures that the output sequence can accommodate `n` characters, reallocating as necessary.
      *
      * \throws std::length_error   if `size() + n > maxSize()`
      *
      * \note The returned buffer is invalidated by any function that modifies the input or output sequences.
      */
    ByteBuf prepare(szt n)                  { reserve(n); return ByteBuf(pptr(), n); }

    /// Move characters from the output sequence to the input sequence
    /**
      * Appends `n` characters from the start of the output sequence to the input
      * sequence. The beginning of the output sequence is advanced by `n` characters.
      *
      * Requires a preceding call `prepare(x)` where `x >= n`, and no intervening
      * operations that modify the input or output sequences.
      *
      * \note If `n` is greater than the size of the output sequence, the entire
      * output sequence is moved to the input sequence and no error is issued.
      */
    void commit(szt n)
    {
        if (pptr() + n > epptr()) n = epptr() - pptr();
        pbump(n);
        setg(eback(), gptr(), pptr());
    }

    /// Remove characters from the input sequence
    /**
      * Removes `n` characters from the beginning of the input sequence.
      *
      * \note If `n` is greater than the size of the input sequence, the entire
      * input sequence is consumed and no error is issued.
      */
    void consume(szt n)
    {
        if (egptr() < pptr()) setg(&_buf[0], gptr(), pptr());
        if (gptr() + n > pptr()) n = pptr() - gptr();
        gbump(n);
    }

protected:
    static const int buf_delta = 128;

    /// Override std::streambuf behaviour
    /**
      * Behaves according to the specification of `std::streambuf::underflow()`.
      */
    int_type underflow()
    {
        if (gptr() < pptr())
        {
            setg(&_buf[0], gptr(), pptr());
            return traits_type::to_int_type(*gptr());
        }
        else
            return traits_type::eof();
    }

    /// Override std::streambuf behaviour
    /**
      * Behaves according to the specification of `std::streambuf::overflow()`,
      * with the specialisation that `std::length_error` is thrown if appending
      * the character to the input sequence would require the condition
      * `size() > maxSize()` to be true.
      */
    int_type overflow(int_type c)
    {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            if (pptr() == epptr())
            {
                szt _bufsize = pptr() - gptr();
                if (_bufsize < _maxSize && _maxSize - _bufsize < buf_delta)
                    reserve(_maxSize - _bufsize);
                else
                    reserve(buf_delta);
            }

            *pptr() = traits_type::to_char_type(c);
            pbump(1);
            return c;
        }

        return traits_type::not_eof(c);
    }

    void reserve(szt n)
    {
        //get current stream positions as offsets
        szt gnext = gptr() - &_buf[0];
        szt pnext = pptr() - &_buf[0];
        szt pend = epptr() - &_buf[0];

        //check if there is already enough space in the put area
        if (n <= pend - pnext) return;

        //shift existing contents of get area to start of buffer
        if (gnext > 0)
        {
            pnext -= gnext;
            std::memmove(&_buf[0], &_buf[0] + gnext, pnext);
        }

        //ensure buffer is large enough to hold at least the specified size
        if (n > pend - pnext)
        {
            if (n <= _maxSize && pnext <= _maxSize - n)
            {
                pend = pnext + n;
                _buf.resize(Alge::max(pend, 1));
            }
            else
                throw std::length_error("buffer too large");
        }

        //update stream positions
        setg(&_buf[0], &_buf[0], &_buf[0] + pnext);
        setp(&_buf[0] + pnext, &_buf[0] + pend);
    }

private:
    szt _maxSize;
    vector<byte, Alloc> _buf;
};

typedef StreamBuf_<> StreamBuf;

} }
