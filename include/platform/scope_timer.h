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
#include <utility>

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
 *     ScopedTimer<MicrosecondsStopwatch, MicrosecondsStopwatch> timer
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
template <class... Listeners>
class ScopeTimer {
public:
    /**
     * Construct a ScopeTimer for N listener objects, taking the arguments
     * for each underlying Listener as a std::tuple.
     *
     * Example:
     *     ScopeTimer<HdrMicroSecStopwatch, TracerStopwatch> timer(
     *         std::forward_as_tuple(stats.getCmdHisto),
     *         std::forward_as_tuple(cookie, cb::tracing::Code::Get));
     *
     * This is admittedly pretty verbose, but it _does_ guarantee that the
     * Listener objects are constructed in-place inside ScopeTimer, so
     * usable for non-movable types, and avoids creating temporary Listener
     * objects which could result in spurious durations being timed.
     */
    template <typename... Args>
    ScopeTimer(Args&&... args)
        // double pack expansion, constructs listener N from argument N
        : listeners{std::make_from_tuple<Listeners>(
                  std::forward<Args>(args))...} {
        // must have one tuple of arguments per Listener. No need for a
        // static_assert, as compilation would fail with (e.g.,):
        //    pack expansion contains parameter packs 'Listeners' and 'args'
        //    that have different lengths (1 vs. 2)

        auto startTime = std::chrono::steady_clock::now();
        // apply to expand the tuple out into N packed arguments
        std::apply(
                [&startTime](auto&... listenersPack) {
                    // call start on every listener (fold expression)
                    (listenersPack.start(startTime), ...);
                },
                listeners);
    }

    // In principle, ScopeTimer could be copiable/movable. However, moving
    // a ScopeTimer<Listener> would currently rely upon it being acceptable to
    // potentially call Listener::stop() on a moved-out-of instance.
    // ScopeTimer could be improved to avoid this but for now, as
    // move/copy construction/assignment is not currently used, explicitly
    // delete them to avoid surprises.
    // Additionally, it's not immediately clear what series of start() and
    // stop() calls to expect when copying a ScopeTimer, another
    // source of potential surprises.
    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer(const ScopeTimer&) = delete;

    ScopeTimer& operator=(ScopeTimer&&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

    ~ScopeTimer() {
        const auto endTime = std::chrono::steady_clock::now();
        std::apply(
                [&endTime](auto&... listenersPack) {
                    // call stop on every listener (fold expression)
                    (listenersPack.stop(endTime), ...);
                },
                listeners);
    }

private:
    std::tuple<Listeners...> listeners;
};
