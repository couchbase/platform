/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc.
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
#ifndef PLATFORM_CBASSERT_H
#define PLATFORM_CBASSERT_H 1

#include <assert.h>

#ifdef NDEBUG
#include <stdio.h>
#include <stdlib.h>

#define cb_assert(e)  \
    ((void)((e) ? 0 : cb_assert_die(#e, __FILE__, __LINE__)))

#define cb_assert_die(e, file, line) \
    ((void)fprintf(stderr, "asssertion failed [%s] at %s:%u\n", \
                   e, file, line), abort())

#else
#define cb_assert(a) assert(a)
#endif

#endif