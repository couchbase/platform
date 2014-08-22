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

/* implementations of string functions missing on some platforms */

#include "config.h"

#include "strings.h"

int asprintf(char **ret, const char *format, ...) {
    va_list ap;
    int count;
    va_start(ap, format);
    count = vasprintf(ret, format, ap);
    va_end(ap);
    return count;
}

int vasprintf(char **ret, const char *format, va_list ap) {
    va_list copy;
    int res;
    va_copy(copy, ap);
    res = vsnprintf(NULL, 0, format, ap);
    if (res >= 0) {
        /* valid format, malloc buffer and do for real */
        char *buffer = malloc(res + 1); /* +1 for \0 */
        res = vsnprintf(buffer, res + 1, format, ap);
        if (res < 0) {
            free(buffer);
        } else {
            *ret = buffer;
        }
    }
    va_end(copy);
    return res;
}
