/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

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
std::string time2text(std::chrono::nanoseconds time);

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
std::chrono::nanoseconds text2time(const std::string& text);
}
