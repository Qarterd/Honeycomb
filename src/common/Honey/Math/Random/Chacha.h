// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/Math/Random/Gen.h"
#include "Honey/String/Bytes.h"

namespace honey
{

/// ChaCha8, a cryptographically secure pseudo random number generator
/**
  * This random generator produces high quality randomness and is also a stream cipher.
  * This class can be used for encryption/decryption because the randomness is generated in a way that
  * a listener to the encrypted message is unable to deduce the initial seed (key+iv).
  *
  * The generator accepts a 256-bit cryptographic key and a 64-bit IV (initialization vector).
  * These can also be combined together to be understood as one 320-bit seed.
  * The entire state of the generator is 512 bits.
  * The period of the generator is 2^70 random integers for each IV.
  *
  * ### Random Number Generation
  * Use setSeed() and next(). \n
  * By default the seed is filled with entropy from the host device.
  *
  * ### Cryptography
  * Use setKey(), setIv() and encrypt() / decrypt(). \n
  * The key should be exchanged and set up once, and the IV should be randomized for each message. \n
  * A unique IV for each message makes the stream more difficult to decipher. \n
  * A separate instance of this class seeded with device entropy can be used to generate keys and IVs.
  *
  *     Sender:                                         Receiver:
  *
  *     key = rand.next(); iv = rand.next();                                        //Generate initial key and IV, use separate Chacha instance
  *     send(key,iv);                                   receive(key,iv);            //Exchange key and IV between sender/receiver
  *     setKey(key); setIv(iv);                         setKey(key); setIv(iv);     //Initialize generator
  *     encrypt(msg,c,len);                             decrypt(c,msg,len);         //Encrypt messages
  *     encrypt(msg2,c,len2);                           decrypt(c,msg2,len2);
  *     iv = rand.next(); encrypt(iv,c,8);              decrypt(c,iv,8);            //Append next IV to cipher
  *     setIv(iv);                                      setIv(iv);                  //Change the IV for the next message
  *     encrypt(msg3,c,len3);                           decrypt(c,msg3,len3);
  */
class Chacha : public RandomGen
{
public:
    /// 256-bit Cryptographic Key
    typedef ByteArray<32> Key;
    /// 64-bit Cryptographic IV
    typedef ByteArray<8> Iv;
    /// 320-bit Seed (Key+IV)
    typedef ByteArray<40> Seed;

    /// Generator State
    struct State
    {
        static const szt coreSize = 16;

        State()                                     : resIdx(coreSize) {}

        /// 512-bit state
        uint32 core[coreSize];
        /// Cached results from step()
        uint32 res[coreSize];
        uint32 resIdx;
    };

    Chacha()                                        { setSeed(); }
    ~Chacha()                                       {}

    /// Set the seed using a default method that gathers entropy from the device
    void setSeed();

    /// Set the random number generator seed
    void setSeed(const Seed& seed);

    /// Generate random number between 0 and 2^64-1 inclusive
    virtual uint64 next();

    /// Init generator with a cryptographic key. 
    void setKey(const Key& key);
    /// Set initialization vector into generator. Call for every message between encrypt/decrypt calls to produce unique streams.
    void setIv(const Iv& iv);

    /// Encrypt a message.  Result is stored in cipher.
    void encrypt(ByteBufConst msg, ByteBuf cipher);
    /// Decrypt a cipher.  Result is stored in msg.
    void decrypt(ByteBufConst cipher, ByteBuf msg);

    /// Set the state of the generator
    void setState(const State& state)               { _state = state; }
    /// Get the current state of the generator. Pass result into setState() to restore a state.
    State& getState()                               { return _state; }

private:
    /// Take one step in the random generator state
    void step();

    State _state;

    static const byte _sigma[];
};

}
