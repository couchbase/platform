/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/checked_snprintf.h>
#include <cstdio>
#include <stdexcept>
#include <system_error>
#include <string>

int checked_snprintf(char* str, const size_t size, const char* format, ...) {
    if (str == nullptr) {
        throw std::invalid_argument(
            "checked_snprintf: destination buffer can't be NULL");
    }

    if (size == 0) {
        throw std::invalid_argument(
            "checked_snprintf: destination buffersize can't be 0");
    }

    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);

    if (ret < 0) {
        // unfortunately this isn't always the case.. but we can't
        // determine the difference on windows so we'll just treat it as an
        // overflow error...
        throw std::overflow_error(
            "checked_snprintf: Destination buffer too small.");
    }

    if (size_t(ret) >= size) {
        std::string msg("checked_snprintf: Destination buffer too small. (");
        msg += std::to_string(ret);
        msg += " >= ";
        msg += std::to_string(size);
        msg += ")";
        throw std::overflow_error(msg);
    }

    return ret;
}
