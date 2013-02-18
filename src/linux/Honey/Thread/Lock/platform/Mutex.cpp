// Honeycomb, Copyright (C) 2013 Daniel Carter.  Distributed under the Boost Software License v1.0.

#include "Honey/Thread/Lock/Mutex.h"

/** \cond */
namespace honey { namespace platform
{

Mutex::Mutex()                          { verify(!pthread_mutex_init(&_handle, nullptr)); }
Mutex::~Mutex()                         { pthread_mutex_destroy(&_handle); }

void Mutex::lock()                      { pthread_mutex_lock(&_handle); }
void Mutex::unlock()                    { pthread_mutex_unlock(&_handle); }
bool Mutex::tryLock()                   { return !pthread_mutex_trylock(&_handle); }

} }
/** \endcond */



