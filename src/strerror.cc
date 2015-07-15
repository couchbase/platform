/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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
#include <platform/platform.h>
#include <platform/strerror.h>

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#include <errno.h>
#endif

PLATFORM_PUBLIC_API
std::string cb_strerror() {
#ifdef WIN32
    return cb_strerror(GetLastError());
#else
    return cb_strerror(errno);
#endif
}

PLATFORM_PUBLIC_API
std::string cb_strerror(cb_os_error_t error)
{
#ifdef WIN32
    std::string reason;
    char *win_msg = NULL;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, error,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&win_msg,
                      0, NULL) > 0) {
        reason.assign(win_msg);
        LocalFree(win_msg);
    } else {
        reason = std::string("Windows error: ") + std::to_string(error);
    }
    return reason;
#else
    return std::string(strerror(error));
#endif
}
