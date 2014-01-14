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
#include <fcntl.h>


PLATFORM_PUBLIC_API
int cb_rand_open(cb_rand_t *handle) {
    *handle = open("/dev/urandom", O_RDONLY);
    return (*handle == -1) ? -1 : 0;
}

PLATFORM_PUBLIC_API
int cb_rand_get(cb_rand_t handle, void *dest, size_t nbytes) {
    return (read(handle, dest, nbytes) == nbytes) ? 0 : -1;
}

PLATFORM_PUBLIC_API
int cb_rand_close(cb_rand_t handle) {
    return close(handle) == -1 ? -1 : 0;
}
