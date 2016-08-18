/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <platform/platform.h>
#include <platform/cbassert.h>
#include <platform/cb_malloc.h>

const char *original = "mktemp_test_XXXXXX";

int main(void) {
    for (int ii = 0; ii < 100; ++ii) {
        char *pattern = cb_strdup(original);
        cb_assert(pattern);

        char *file = cb_mktemp(pattern);
        /* cb_mktemp _could_ fail, but then we might have other and bigger
         * problems.
         */
        cb_assert(file);
        cb_assert(file == pattern);
        cb_assert(strcmp(pattern, original) != 0);

        /* the file should exist.. lets try to read it */
        FILE *fp = fopen(file, "r");
        cb_assert(fp != NULL);
        fclose(fp);

        remove(pattern);
        cb_free(pattern);
    }

    char *pattern = cb_strdup("foo");
    cb_assert(pattern);
    cb_assert(cb_mktemp(pattern) == NULL);
    cb_free(pattern);

    return 0;
}
