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
#pragma once

#include <platform/platform.h>
#include <chrono>
#include <string>

namespace cb {
/**
 * Convert a time (in ns) to a human readable form...
 *
 * Up to 9999ns, print as ns
 * up to 9999µs, print as µs
 * up to 9999ms, print as ms
 * up to 599s, print as s
 *
 * Anything else is printed as h:m:s
 *
 * @param time the time in nanoseconds
 * @return a string representation of the timestamp
 */
PLATFORM_PUBLIC_API
std::string time2text(const std::chrono::nanoseconds time);

/**
 * Try to parse the string. It should be of the following format:
 *
 * value [specifier]
 *
 * Ex: "5 s" or "5s" or "5 seconds"
 *
 * where the specifier may be:
 *    ns / nanoseconds
 *    us / microseconds
 *    ms / milliseconds
 *    s / seconds
 *    m / minutes
 *    h / hours
 *
 * If no specifier is provided, the value specifies the number in milliseconds
 */
PLATFORM_PUBLIC_API std::chrono::nanoseconds text2time(const std::string& text);
}
