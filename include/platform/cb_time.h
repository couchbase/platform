/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <atomic>
#include <chrono>

namespace cb::time {

struct steady_clock {
    using duration = std::chrono::steady_clock::duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::steady_clock::time_point;
    static constexpr bool is_steady = true;

    /**
     * @return time_point for now from a steady clock, either std::chrono or our
     *         static variant (which only ticks when requested via advance).
     */
    static time_point now() {
        if (use_chrono) {
            return std::chrono::steady_clock::now();
        }
        return static_now();
    }

    /**
     * @return a time_point which begins as std::chrono::steady_clock::now and
     * only advances with calls to advance
     */
    static time_point static_now();

    /**
     * Advance the static clock - function performs time += duration
     * @param duration to advance clock by
     */
    static void advance(std::chrono::nanoseconds duration);

    /**
     * Flag controlling what steady_clock::now returns. When true (the default)
     * steady_clock::now will return the value of std::chrono::steady_clock::now
     * and when false steady_clock::now will return the value a value which only
     * advances with calls to advance(duration).
     */
    static std::atomic<bool> use_chrono;
};

/**
 * RAII helper to switch cb::time::steady_clock to static mode for a test
 * and restore it afterwards.
 */
class StaticClockGuard {
public:
    StaticClockGuard() : wasUsingChrono(cb::time::steady_clock::use_chrono) {
        cb::time::steady_clock::use_chrono = false;
    }
    ~StaticClockGuard() {
        cb::time::steady_clock::use_chrono = wasUsingChrono;
    }

private:
    const bool wasUsingChrono;
};

} // end namespace cb::time