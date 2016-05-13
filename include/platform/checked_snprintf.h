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
#pragma once

#include <cstdarg>
#include <platform/platform.h>

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
PLATFORM_PUBLIC_API
int checked_snprintf(char* str, const size_t size, const char* format, ...);
