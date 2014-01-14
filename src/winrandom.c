/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include "config.h"

#include <platform/random.h>

PLATFORM_PUBLIC_API
int cb_rand_open(cb_rand_t *handle) {
    if (!CryptAcquireContext(handle, "Couchbase", NULL, PROV_RSA_FULL, 0)) {
        if (GetLastError() != NTE_BAD_KEYSET) {
            return -1;
        }

        /* Try to create a new keyset */
        if (!CryptAcquireContext(handle, "Couchbase", NULL,
                                 PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
            return -1;
        }
    }

    return 0;
}

PLATFORM_PUBLIC_API
int cb_rand_get(cb_rand_t handle, void *dest, size_t nbytes) {
    return CryptGenRandom(handle, (DWORD)nbytes, dest) ? 0 : -1;
}

PLATFORM_PUBLIC_API
int cb_rand_close(cb_rand_t handle) {
    return CryptReleaseContext(handle, 0) ? 0 : -1;
}
