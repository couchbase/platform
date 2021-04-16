/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc.
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
