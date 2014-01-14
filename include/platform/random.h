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
#pragma once
#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

#include <platform/visibility.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef HCRYPTPROV cb_rand_t;
#else
    typedef int cb_rand_t;
#endif
    /*
     * Upon errors windows users should look at GetLastError, and
     * others should look at errno if they want more detailed
     * information.
     */

    /**
     * Open a random generator
     *
     * @param handle pointer to the handle to open
     * @param return 0 on success -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_rand_open(cb_rand_t *handle);

    /**
     * Get random bytes from the random generator
     *
     * @param handle the handle used to read data from
     * @param dest where to store the random bytes
     * @param nbytes the number of bytes to get
     * @param return 0 on success -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_rand_get(cb_rand_t handle, void *dest, size_t nbytes);

    /**
     * Close a random generator
     *
     * @param handle The random generator to close
     */
    PLATFORM_PUBLIC_API
    int cb_rand_close(cb_rand_t handle);

#ifdef __cplusplus
}
#endif
