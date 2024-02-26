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
#include <tuple>

/**
 * Timer wrappers which provides a RAII-style mechanism for timing the duration
 * of sections of code; where multiple listeners can record the same duration.
 *
 * The motivation for this class is we have regions of code which multiple
 * listeners want to time. We don't want to just have each listener perform
 * it's reading of the clock as that is (a) costly and (b) gives slightly
 * different time values to each listener. Instead, this class handles reading
 * the time (just once at start and stop); and then passes it onto each
 * listener.
 *
 * On construction, it in-place constructs each of the Listener template
 * parmeters, reads the current time of the std::chrono::steady_clock; and calls
 * the start() method of each listener object. On destruction (i.e. when this
 * object goes out of scope), reads std::chrono::steady_clock::now a second
 * time, calls end() on each listener object, and then destructs the listeners.
 *
 * Example usage:
 *
 * {
 *     ... some scope we wish to time
 *     ScopedTimer2<MicrosecondsStopwatch, MicrosecondsStopwatch> timer
 *             std::forward_as_tuple(stats.histogram1),
 *             std::forward_as_tuple(stats.histogram2));
 *     // start() called on both MicrosecondStopwatches
 *     ...
 *  } // at end of scope stop() called on both MicrosecondStopwatches
 *
 * Note: Listeners are constructed in-place inside the ScopeTimer objects,
 *       which avoids having to worry about temporary Listener objects being
 *       created which could create spurious timer events
 *
 * Note(2): This initial implementation has a simple, dumb implementation of
 *       different classes for each count of listeners. It could well be
 *       possible to do something clever with variadic templates to genericize
 *       this, but given we only ever use either 1 or 2 listeners that seems
 *       overkill at the present time :)
 */

/**
 * ScopeTimer taking one listener objects. Somewhat unnecessary, but provided
 * for consistency / ease of moving between an N arg variant.
 */
template <class Listener1>
class ScopeTimer1 {
public:
    template <typename... Args>
    ScopeTimer1(Args&&... args) : listener1(std::forward<Args>(args)...) {
        listener1.start(std::chrono::steady_clock::now());
    }

    ~ScopeTimer1() {
        const auto endTime = std::chrono::steady_clock::now();
        listener1.stop(endTime);
    }

private:
    Listener1 listener1;
};

/// ScopeTimer taking two listener objects.
template <class Listener1, class Listener2>
class ScopeTimer2 {
public:
    /**
     * Construct a ScopeTimer for two listener objects, taking the arguments
     * for each underlying Listener as a std::tuple.
     *
     * Example:
     *     ScopeTimer2<HdrMicroSecStopwatch, TracerStopwatch> timer(
     *         std::forward_as_tuple(stats.getCmdHisto),
     *         std::forward_as_tuple(cookie, cb::tracing::Code::Get));
     *
     * This is admittedly pretty verbose, but it _does_ guarantee that the
     * Listener objects are constructed in-place inside ScopeTimer2, so
     * usable for non-movable types, and avoids creating temporary Listener
     * objects which could result in spurious durations being timed.
     */
    template <typename Args1, typename Args2>
    ScopeTimer2(Args1&& args1, Args2&& args2)
        : listener1(
                  std::make_from_tuple<Listener1>(std::forward<Args1>(args1))),
          listener2(
                  std::make_from_tuple<Listener2>(std::forward<Args2>(args2))) {
        auto startTime = std::chrono::steady_clock::now();
        listener1.start(startTime);
        listener2.start(startTime);
    }

    ~ScopeTimer2() {
        const auto endTime = std::chrono::steady_clock::now();
        listener1.stop(endTime);
        listener2.stop(endTime);
    }

private:
    Listener1 listener1;
    Listener2 listener2;
};
