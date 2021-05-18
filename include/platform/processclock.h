/*
 *     Copyright 2018-Present Couchbase, Inc.
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

// Simple wrapper function that returns std::chrono::nanoseconds
// given a std::chrono::steady_clock::time_point
std::chrono::nanoseconds to_ns_since_epoch(
        const std::chrono::steady_clock::time_point& tp);

/**
 * Interface for a source of 'now' for the std::chrono::steady_clock to allow
 * for dependency injection of time.
 */
struct ProcessClockSource {
    virtual std::chrono::steady_clock::time_point now() = 0;
    virtual ~ProcessClockSource() = default;
};

/**
 * Default 'now' source for std::chrono::steady_clock, simply
 * proxies std::chrono::steady_clock::now()
 */
struct DefaultProcessClockSource : ProcessClockSource {
    std::chrono::steady_clock::time_point now() override;
};

/**
 * Singleton instance of DefaultProcessClockSource
 */
DefaultProcessClockSource& defaultProcessClockSource();
}

inline std::chrono::nanoseconds to_ns_since_epoch(
        const std::chrono::steady_clock::time_point& tp) {
    return cb::to_ns_since_epoch(tp);
}
