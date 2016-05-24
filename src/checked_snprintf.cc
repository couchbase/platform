/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

#include <platform/checked_snprintf.h>
#include <cstdio>
#include <stdexcept>
#include <system_error>
#include <string>

PLATFORM_PUBLIC_API
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
    } else if (size_t(ret) >= size) {
        std::string msg("checked_snprintf: Destination buffer too small. (");
        msg += std::to_string(ret);
        msg += " >= ";
        msg += std::to_string(size);
        msg += ")";
        throw std::overflow_error(msg);
    }

    return ret;
}
