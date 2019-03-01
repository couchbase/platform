/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc
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

#include <platform/dynamic.h>
#include <platform/visibility.h>
#include <platform/cbassert.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef long ssize_t;
#define DIRECTORY_SEPARATOR_CHARACTER '\\'
#else
#define DIRECTORY_SEPARATOR_CHARACTER '/'
#endif // WIN32

    /*
     * Set mode to binary
     */
    PLATFORM_PUBLIC_API
    int platform_set_binary_mode(FILE* fp);

    /**
     * Some of our platforms complain on not using mkstemp. Instead of
     * having the test in all programs we're just going to use this
     * method instead.
     *
     * @param pattern The input pattern for the filename. It should end
     *                with six X's that will be replaced with the unique
     *                filename. The file will be created.
     * @return pattern on success, NULL upon failure. Check errno for
     *                 the reason.
     */
    PLATFORM_PUBLIC_API
    char* cb_mktemp(char* pattern);

#ifdef __cplusplus
}
#endif
