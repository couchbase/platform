/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/strerror.h>

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#include <errno.h>
#endif

std::string cb_strerror() {
#ifdef WIN32
    return cb_strerror(GetLastError());
#else
    return cb_strerror(errno);
#endif
}

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
