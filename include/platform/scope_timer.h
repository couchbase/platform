/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <platform/processclock.h>

/**
 * Timer wrappers which provides a RAII-style mechanism for timing the duration
 * of sections of code; where multiple listeners can record the same duration.
 *
 * The motiviation for this class is we have regions of code which multiple
 * listeners want to time. We don't want to just have each listener perform
 * it's reading of the clock as that is (a) costly and (b) gives slightly
 * different time values to each listener. Instead, this class handles reading
 * the time (just once at start and stop); and then passes it onto each
 * listener.
 *
 * On construction, it reads the current time of the ProcessClock; and calls the
 * start() method in each passed in listener object.
 * On destruction (i.e. when this object goes out of scope), reads
 * ProcessClock::now a second time, and calls end() on each listener object.
 *
 * Example usage:
 *
 * {
 *     ... some scope we wish to time
 *     ScopedTimer2<MicrosecondsStopwatch, MicrosecondsStopwatch> timer
 *             (MicrosecondStopwatch(stats.histogram1),
 *             (MicrosecondStopwatch(stats.histogram2));
 *     // start() called on MicrosecondStopwatch
 *     ...
 *  } // at end of scope stop() called on MicrosecondStopwatch.
 *
 * Note: The listener objects are passed by rvalue-reference; this is helpful
 *       as it means we can move them into the ScopeTimer object (and hence
 *       keep them around to call stop() on). However this doesn't implement
 *       perfect forwarding, so you should ensure the Listener object can be
 *       copied/moved safely.
 *
 * Note(2): This initial implementation has a simple, dumb implementation of
 *       different classes for each count of listeners. It could well be
 *       possible to do something clever with variadic templates to genericize
 *       this, but given we only ever use either 1 or 2 listeners that seems
 *       overkill at the present time :)
 */
template <class Listener1>
class ScopeTimer1 {
public:
    ScopeTimer1(Listener1&& listener1)
        : startTime(ProcessClock::now()),
          listener1(std::forward<Listener1>(listener1)) {
        const auto startTime = ProcessClock::now();
        listener1.start(startTime);
    }

    ~ScopeTimer1() {
        const auto endTime = ProcessClock::now();
        listener1.stop(endTime);
    }

private:
    const ProcessClock::time_point startTime;
    Listener1 listener1;
};

/// ScopeTimer taking two listener objects.
template <class Listener1, class Listener2>
class ScopeTimer2 {
public:
    ScopeTimer2(Listener1&& listener1, Listener2&& listener2)
        : startTime(ProcessClock::now()),
          listener1(std::forward<Listener1>(listener1)),
          listener2(std::forward<Listener2>(listener2)) {
        listener1.start(startTime);
        listener2.start(startTime);
    }

    ~ScopeTimer2() {
        const auto endTime = ProcessClock::now();
        listener1.stop(endTime);
        listener2.stop(endTime);
    }

private:
    const ProcessClock::time_point startTime;
    Listener1 listener1;
    Listener2 listener2;
};
