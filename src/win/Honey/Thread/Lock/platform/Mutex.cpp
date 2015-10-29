// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.
#pragma hdrstop
#include "Honey/Thread/Lock/Mutex.h"

/** \cond */
namespace honey { namespace platform
{

Mutex::Mutex()                                              { InitializeCriticalSection(&_handle); }
Mutex::~Mutex()                                             { DeleteCriticalSection(&_handle); }

void Mutex::lock()                                          { EnterCriticalSection(&_handle); }
void Mutex::unlock()                                        { LeaveCriticalSection(&_handle); }
bool Mutex::tryLock()                                       { return TryEnterCriticalSection(&_handle); }  

} }
/** \endcond */



