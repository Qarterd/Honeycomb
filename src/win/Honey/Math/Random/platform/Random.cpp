// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "Honey/Math/Random/platform/Random.h"
#include "Honey/Misc/Exception.h"
#include <wincrypt.h>

/** \cond */
namespace honey { namespace random { namespace platform
{

Bytes deviceEntropy(szt count)
{
    Bytes rand(count);
    HCRYPTPROV hProvider = NULL;
    if (!CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, 0))
    {
        // failed, should we try to create a default provider?
        if (NTE_BAD_KEYSET == GetLastError())
            CryptAcquireContext(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    }
    if (!hProvider) throw_ Exception() << "Unable to generate device entropy";

    CryptGenRandom(hProvider, rand.size(), rand.data());
    CryptReleaseContext(hProvider, 0U);
    return rand;
}

} } }
/** \endcond */