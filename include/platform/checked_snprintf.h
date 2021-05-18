/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/dynamic.h>
#include <cstdarg>
#include <cstddef>

/**
 * This is a checked version of snprintf that will throw exceptions. It
 * allows you to do stuff like:
 *
 * <code>
 *    int offset = checked_snprintf(...)
 *    offset += checked_snprintf(...)
 * </code>
 *
 * Unfortunately the behaviour of snprintf differs between the platforms.
 * On Windows snprintf returns -1 if snprintf fails or the provided buffer
 * is too small to contain the entire formatted message (dest has to
 * be a non-null value for this to be true. if dest is set to null, it
 * returns the length of the formatted string). On other platforms snprintf
 * returns -1 on error and then the number of bytes the entire formatted
 * string would take (if there was room for it), but not write outside
 * the provided buffer. Since we can't cleanly determine between the
 * two error scenarios on windows, I'm treating all errors from
 * the underlying snprintf the same and throw std::overflow_error.
 * (the only times I've seen snprintf fail is when it failed to allocate
 * memory anyway)
 *
 * @param str destination buffer
 * @param size the size of the destination buffer
 * @param format the format string
 * @param ... the optional arguments (just like snprintf)
 * @returns the number of bytes added to the output buffer
 * @throws std::invalid_argument if str == 0 or size == 0
 * @throws std::overflow_error if the destination buffer isn't big enough,
 *                             (or snprintf returned -1)
 */
int checked_snprintf(char* str, const size_t size, const char* format, ...)
CB_FORMAT_PRINTF(3, 4);
