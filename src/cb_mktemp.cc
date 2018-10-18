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
#include "config.h"

#include <string.h>
#include <chrono>
#include <cinttypes>

#ifdef WIN32
#include <io.h> /* for mktemp*/
#else

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#endif

PLATFORM_PUBLIC_API
char* cb_mktemp(char* pattern) {
    int searching = 1;
    char* ptr = strstr(pattern, "XXXXXX");
    if (ptr == NULL) {
        return NULL;
    }
    auto counter = std::chrono::steady_clock::now().time_since_epoch().count();

    do {
        ++counter;
        sprintf(ptr, "%06" PRIu64, static_cast<uint64_t>(counter) % 1000000);

#ifdef WIN32
        HANDLE handle = CreateFile(pattern,
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            searching = 0;
        }
#else
        int fd = open(pattern, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd != -1) {
            close(fd);
            searching = 0;
        }
#endif

    } while (searching);

    return pattern;
}
