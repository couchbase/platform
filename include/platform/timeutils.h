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

/**
 * Format a time-point into the following format:
 *
 * YYYY-MM-DDThh:mm::ss.uuuuuu[timezone]
 *
 * In UTC it'll look something like: 2023-10-03T02:36:00.000000Z
 * In PST it'll look something like: 2023-10-03T02:36:00.000000-07:00
 */
std::string to_string(std::chrono::system_clock::time_point tp);

namespace cb {
namespace time {
/// Generate a timestamp for the provided (or current) time
std::string timestamp(std::chrono::system_clock::time_point tp =
                              std::chrono::system_clock::now());
/// Generate a timestamp from a time_t with an optional microseconds delta
std::string timestamp(time_t tp, uint32_t microseconds = 0);
/// Generate a timestamp with a number of nanoseconds form epoch
std::string timestamp(std::chrono::nanoseconds time_since_epoc);
} // namespace time

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

struct ClockOverheadResult {
    /**
     * The mean overhead to call Clock::now() over the specified number of
     * samples.
     */
    std::chrono::nanoseconds overhead;

    /**
     * The period of the base clock used to benchmark the target Clock - i.e.
     * 'overhead' field is only measured to within this value.
     * Note this is logically 'constexpr' - it will always be the same across
     * an instance of the process.
     */
    std::chrono::nanoseconds measurementPeriod;
};

/**
 * Estimates the overhead of measuring the current time with the given clock -
 * returns the time taken to perform ClockUnderTest::now() over a number of
 * samples.
 *
 * @tparam ClockUnderTest The clock to benchmark
 * @tparam MeasuringClock The clock to use to measure elapsed time between
 *         sampleCount calls to ClockUnderTest::now()
 * @param sampleCount The number of samples of ClockUnderTest::now() to
          measure.
 * @returns A ClockOverheadResult for the benchmarked Clock.
 *
 * Note on estimate accuracy:
 *       The accuracy of the estimate is significantly affected by the overhead
 *       of MeasuringClock vs ClockUnderTest - ideally MeasuringClock should
 *       be (much) lower overhead than ClockUnderTest. However this is not
 *       always possible (or even known to the caller).
 *       Such a large difference in overhead can be mitigated by using a
 *       sufficiently large sampleCount - 2 calls to MeasuringClock::now() are
 *       made for every 'sampleCount' calls to ClockUnderTest::now();
 *       however a larger sampleCount will require longer to run.
 *
 * Example real-world values:
 *       The default MeasuringClock (std::chrono::steady_clock) on a recent
 *       CPU (e.g circa 2018 Intel i9-8950HK) with correct OS settings can
 *       be very fast. However various factors can make that significantly
 *       slower:
 *       - clocksource=tsc: 30ns
 *       - clocksource=hpet: 7000ns
 *         (This was observed when running Linux VM in macOS Docker and the
 *         kernel detected too much jitter in default tsc clocksource;
 *         and fell back to HPET).
 */
template <class ClockUnderTest, class MeasuringClock = std::chrono::steady_clock>
ClockOverheadResult estimateClockOverhead(int sampleCount = 1000) {
    // Q: How to estimate the overhead of Clock::now()?
    // One approach would be call it back-to-back, then take the difference
    // of the two results. For a better estimate we can take multiple samples
    // and calculate the average (mean).
    // That's fine if the clock has a sufficiently precise period (say
    // nanoseconds), but is problematic if the clock has a coarser period and
    // hence two consecutive calls to now() return the same value.
    // folly::chrono::coarse_steady_clock (wrapper on CLOCK_MONOTONIC_COARSE)
    // fits this description; its period is milliseconds but it typically
    // takes just a few nanoseconds to read, as such this approach will just
    // return an overhead of 0.
    // Instead, we do something a little more complex - we use a separate clock
    // (which should have as fast a period as possible) to measure the time
    // before and after N * Clock::now() calls, returning the difference
    // between before and after, divided by N.
    // One limitation here is the overhead / jitter of the Measuring clock -
    // which unfortunately can be significant if using a "slow" OS clocksource
    // such as HPET. For example if the Measuring clock is 100x slower to
    // access than the ClockUnderTest (which it can be for MeasuringClock ==
    // steady_clock and ClockUnderTest == coarse_steady_clock) then then for
    // 100 samples we'd end up getting an estimte for ClockUnderTest::now()
    // 2x greater than it really is.
    // Simples ;)
    const auto start = MeasuringClock::now();
    typename ClockUnderTest::time_point cutNow;
    for (auto i = 0; i < sampleCount; i++) {
        cutNow = ClockUnderTest::now();
    }
    typename MeasuringClock::time_point end;
    if constexpr(std::is_same_v<ClockUnderTest, MeasuringClock>) {
        // Using same types for ClockUnderTest and MeasuringClock -
        // can optimise a little and make use of the value from the
        // ClockUnderTest::now for 'end' given it's just as precise as
        // MeasuringClock - this essentially removes the jitter /
        // overhead from 1 call to now().
        end = cutNow;
    } else {
        end = MeasuringClock::now();
    }

    const auto mean = ((end - start) / sampleCount);
    return {mean, typename MeasuringClock::duration{1}};
}

}
