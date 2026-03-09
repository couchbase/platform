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
#include <cerrno>
#include <system_error>

std::string cb_strerror() {
#ifdef WIN32
    return cb_strerror(GetLastError());
#else
    return cb_strerror(errno);
#endif
}

std::string cb_strerror(cb_os_error_t error) {
    return std::system_category().message(error);
}
