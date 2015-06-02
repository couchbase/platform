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
#pragma once

#include <assert.h>
#include <platform/visibility.h>

#ifdef __cplusplus
extern "C" {
#endif
    PLATFORM_PUBLIC_API
    void cb_assert_die(const char *expression, const char *file, int line);
#ifdef __cplusplus
}
#endif

#define cb_assert(e)  \
    ((void)((e) ? (void)0 : cb_assert_die(#e, __FILE__, __LINE__)))
