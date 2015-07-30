// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Chacha.h"
#include "Honey/Math/Random/Random.h"
#include "Honey/String/Hash.h"
#include "Honey/Thread/Atomic.h"

namespace honey
{

#define ROTATE(v,c) (BitOp::rotLeft(v,c))
#define XOR(v,w)    ((v) ^ (w))
#define PLUS(v,w)   ((v) + (w))
#define PLUSONE(v)  (PLUS((v),1))

#define QUARTERROUND(a,b,c,d) \
    a = PLUS(a,b); d = ROTATE(XOR(d,a),16); \
    c = PLUS(c,d); b = ROTATE(XOR(b,c),12); \
    a = PLUS(a,b); d = ROTATE(XOR(d,a), 8); \
    c = PLUS(c,d); b = ROTATE(XOR(b,c), 7);
  
const byte Chacha::_sigma[] = "expand 32-byte k";

void Chacha::setSeed()
{
    setSeed(random::deviceEntropy(mt::arraySize<Seed>::value));
}

void Chacha::setSeed(const Seed& seed)
{
    //Key
    _state.core[1] = BitOp::fromPartsBig<uint32>(seed.data() + 0*sizeof(uint32));
    _state.core[2] = BitOp::fromPartsBig<uint32>(seed.data() + 1*sizeof(uint32));
    _state.core[3] = BitOp::fromPartsBig<uint32>(seed.data() + 2*sizeof(uint32));
    _state.core[4] = BitOp::fromPartsBig<uint32>(seed.data() + 3*sizeof(uint32));
    _state.core[11] = BitOp::fromPartsBig<uint32>(seed.data() + 4*sizeof(uint32));
    _state.core[12] = BitOp::fromPartsBig<uint32>(seed.data() + 5*sizeof(uint32));
    _state.core[13] = BitOp::fromPartsBig<uint32>(seed.data() + 6*sizeof(uint32));
    _state.core[14] = BitOp::fromPartsBig<uint32>(seed.data() + 7*sizeof(uint32));
    //Constants
    _state.core[0] = BitOp::fromPartsLittle<uint32>(_sigma + 0*sizeof(uint32));
    _state.core[5] = BitOp::fromPartsLittle<uint32>(_sigma + 1*sizeof(uint32));
    _state.core[10] = BitOp::fromPartsLittle<uint32>(_sigma + 2*sizeof(uint32));
    _state.core[15] = BitOp::fromPartsLittle<uint32>(_sigma + 3*sizeof(uint32));
    //IV
    _state.core[6] = BitOp::fromPartsBig<uint32>(seed.data() + 8*sizeof(uint32));
    _state.core[7] = BitOp::fromPartsBig<uint32>(seed.data() + 9*sizeof(uint32));
    //Block counter
    _state.core[8] = 0;
    _state.core[9] = 0;

    //Advance a number of iterations to remove any random bias
    for (int i = 0; i < 10; ++i) step();
}

void Chacha::step()
{
    uint32 x0  = _state.core[0];
    uint32 x1  = _state.core[1];
    uint32 x2  = _state.core[2];
    uint32 x3  = _state.core[3];
    uint32 x4  = _state.core[4];
    uint32 x5  = _state.core[5];
    uint32 x6  = _state.core[6];
    uint32 x7  = _state.core[7];
    uint32 x8  = _state.core[8];
    uint32 x9  = _state.core[9];
    uint32 x10 = _state.core[10];
    uint32 x11 = _state.core[11];
    uint32 x12 = _state.core[12];
    uint32 x13 = _state.core[13];
    uint32 x14 = _state.core[14];
    uint32 x15 = _state.core[15];

    for (int i = 8; i > 0; i -= 2)
    {
        QUARTERROUND( x0, x4, x8,x12)
        QUARTERROUND( x1, x5, x9,x13)
        QUARTERROUND( x2, x6,x10,x14)
        QUARTERROUND( x3, x7,x11,x15)
        QUARTERROUND( x0, x5,x10,x15)
        QUARTERROUND( x1, x6,x11,x12)
        QUARTERROUND( x2, x7, x8,x13)
        QUARTERROUND( x3, x4, x9,x14)
    }

    _state.res[ 0] = PLUS(x0 ,_state.core[ 0]);
    _state.res[ 1] = PLUS(x1 ,_state.core[ 1]);
    _state.res[ 2] = PLUS(x2 ,_state.core[ 2]);
    _state.res[ 3] = PLUS(x3 ,_state.core[ 3]);
    _state.res[ 4] = PLUS(x4 ,_state.core[ 4]);
    _state.res[ 5] = PLUS(x5 ,_state.core[ 5]);
    _state.res[ 6] = PLUS(x6 ,_state.core[ 6]);
    _state.res[ 7] = PLUS(x7 ,_state.core[ 7]);
    _state.res[ 8] = PLUS(x8 ,_state.core[ 8]);
    _state.res[ 9] = PLUS(x9 ,_state.core[ 9]);
    _state.res[10] = PLUS(x10,_state.core[10]);
    _state.res[11] = PLUS(x11,_state.core[11]);
    _state.res[12] = PLUS(x12,_state.core[12]);
    _state.res[13] = PLUS(x13,_state.core[13]);
    _state.res[14] = PLUS(x14,_state.core[14]);
    _state.res[15] = PLUS(x15,_state.core[15]);

    //Increase block counter. Stopping at 2^70 bytes per IV is user's responsibility
    _state.core[8] = PLUSONE(_state.core[8]);
    if (!_state.core[8])
        _state.core[9] = PLUSONE(_state.core[9]);

    //Reset index of next integer
    _state.resIdx = 0;
}

uint64 Chacha::next()
{
    //Every generator step produces coreSize count of random 32-bit integers
    if (_state.resIdx >= _state.coreSize)
        step();

    uint64 res = BitOp::fromPartsLittle(_state.res + _state.resIdx);
    _state.resIdx += 2;
    return res;
}

void Chacha::setKey(const Key& key)
{
    //Key
    _state.core[1] = BitOp::fromPartsBig<uint32>(key.data() + 0*sizeof(uint32));
    _state.core[2] = BitOp::fromPartsBig<uint32>(key.data() + 1*sizeof(uint32));
    _state.core[3] = BitOp::fromPartsBig<uint32>(key.data() + 2*sizeof(uint32));
    _state.core[4] = BitOp::fromPartsBig<uint32>(key.data() + 3*sizeof(uint32));
    _state.core[11] = BitOp::fromPartsBig<uint32>(key.data() + 4*sizeof(uint32));
    _state.core[12] = BitOp::fromPartsBig<uint32>(key.data() + 5*sizeof(uint32));
    _state.core[13] = BitOp::fromPartsBig<uint32>(key.data() + 6*sizeof(uint32));
    _state.core[14] = BitOp::fromPartsBig<uint32>(key.data() + 7*sizeof(uint32));
    //Constants
    _state.core[0] = BitOp::fromPartsLittle<uint32>(_sigma + 0*sizeof(uint32));
    _state.core[5] = BitOp::fromPartsLittle<uint32>(_sigma + 1*sizeof(uint32));
    _state.core[10] = BitOp::fromPartsLittle<uint32>(_sigma + 2*sizeof(uint32));
    _state.core[15] = BitOp::fromPartsLittle<uint32>(_sigma + 3*sizeof(uint32));
}

void Chacha::setIv(const Iv& iv)
{
    //IV
    _state.core[6] = BitOp::fromPartsBig<uint32>(iv.data() + 0*sizeof(uint32));
    _state.core[7] = BitOp::fromPartsBig<uint32>(iv.data() + 1*sizeof(uint32));
    //Block counter
    _state.core[8] = 0;
    _state.core[9] = 0;
}

void Chacha::encrypt(const byte* m, byte* c, int len)
{
    uint32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    uint32 j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
    byte* ctarget = 0;
    byte tmp[64];

    if (len <= 0) return;

    j0 = _state.core[0];
    j1 = _state.core[1];
    j2 = _state.core[2];
    j3 = _state.core[3];
    j4 = _state.core[4];
    j5 = _state.core[5];
    j6 = _state.core[6];
    j7 = _state.core[7];
    j8 = _state.core[8];
    j9 = _state.core[9];
    j10 = _state.core[10];
    j11 = _state.core[11];
    j12 = _state.core[12];
    j13 = _state.core[13];
    j14 = _state.core[14];
    j15 = _state.core[15];

    for (;;)
    {
        if (len < 64)
        {
            for (int i = 0; i < len; ++i)
                tmp[i] = m[i];
            m = tmp;
            ctarget = c;
            c = tmp;
        }

        x0 = j0;
        x1 = j1;
        x2 = j2;
        x3 = j3;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;

        for (int i = 8; i > 0; i -= 2)
        {
            QUARTERROUND( x0, x4, x8,x12)
            QUARTERROUND( x1, x5, x9,x13)
            QUARTERROUND( x2, x6,x10,x14)
            QUARTERROUND( x3, x7,x11,x15)
            QUARTERROUND( x0, x5,x10,x15)
            QUARTERROUND( x1, x6,x11,x12)
            QUARTERROUND( x2, x7, x8,x13)
            QUARTERROUND( x3, x4, x9,x14)
        }

         x0 = PLUS( x0,j0);
         x1 = PLUS( x1,j1);
         x2 = PLUS( x2,j2);
         x3 = PLUS( x3,j3);
         x4 = PLUS( x4,j4);
         x5 = PLUS( x5,j5);
         x6 = PLUS( x6,j6);
         x7 = PLUS( x7,j7);
         x8 = PLUS( x8,j8);
         x9 = PLUS( x9,j9);
        x10 = PLUS(x10,j10);
        x11 = PLUS(x11,j11);
        x12 = PLUS(x12,j12);
        x13 = PLUS(x13,j13);
        x14 = PLUS(x14,j14);
        x15 = PLUS(x15,j15);

         x0 = XOR( x0,BitOp::fromPartsLittle<uint32>(m + 0));
         x1 = XOR( x1,BitOp::fromPartsLittle<uint32>(m + 4));
         x2 = XOR( x2,BitOp::fromPartsLittle<uint32>(m + 8));
         x3 = XOR( x3,BitOp::fromPartsLittle<uint32>(m + 12));
         x4 = XOR( x4,BitOp::fromPartsLittle<uint32>(m + 16));
         x5 = XOR( x5,BitOp::fromPartsLittle<uint32>(m + 20));
         x6 = XOR( x6,BitOp::fromPartsLittle<uint32>(m + 24));
         x7 = XOR( x7,BitOp::fromPartsLittle<uint32>(m + 28));
         x8 = XOR( x8,BitOp::fromPartsLittle<uint32>(m + 32));
         x9 = XOR( x9,BitOp::fromPartsLittle<uint32>(m + 36));
        x10 = XOR(x10,BitOp::fromPartsLittle<uint32>(m + 40));
        x11 = XOR(x11,BitOp::fromPartsLittle<uint32>(m + 44));
        x12 = XOR(x12,BitOp::fromPartsLittle<uint32>(m + 48));
        x13 = XOR(x13,BitOp::fromPartsLittle<uint32>(m + 52));
        x14 = XOR(x14,BitOp::fromPartsLittle<uint32>(m + 56));
        x15 = XOR(x15,BitOp::fromPartsLittle<uint32>(m + 60));

        //Increase block counter. Stopping at 2^70 bytes per IV is user's responsibility
        j8 = PLUSONE(j8);
        if (!j8)
          j9 = PLUSONE(j9);

        BitOp::toPartsLittle( x0,c + 0);
        BitOp::toPartsLittle( x1,c + 4);
        BitOp::toPartsLittle( x2,c + 8);
        BitOp::toPartsLittle( x3,c + 12);
        BitOp::toPartsLittle( x4,c + 16);
        BitOp::toPartsLittle( x5,c + 20);
        BitOp::toPartsLittle( x6,c + 24);
        BitOp::toPartsLittle( x7,c + 28);
        BitOp::toPartsLittle( x8,c + 32);
        BitOp::toPartsLittle( x9,c + 36);
        BitOp::toPartsLittle(x10,c + 40);
        BitOp::toPartsLittle(x11,c + 44);
        BitOp::toPartsLittle(x12,c + 48);
        BitOp::toPartsLittle(x13,c + 52);
        BitOp::toPartsLittle(x14,c + 56);
        BitOp::toPartsLittle(x15,c + 60);

        if (len <= 64)
        {
            if (len < 64)
                for (int i = 0; i < len; ++i)
                    ctarget[i] = c[i];
            _state.core[8] = j8;
            _state.core[9] = j9;
            break;
        }
        len -= 64;
        c += 64;
        m += 64;
    }

    //Reset index, state is dirty. A call to get a random number from next() will require a new step
    _state.resIdx = _state.coreSize;
}

void Chacha::decrypt(const byte* cipher, byte* msg, int len)
{
    encrypt(cipher, msg, len);
}

}
