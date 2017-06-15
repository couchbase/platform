/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
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

#include <platform/processclock.h>

cb::ProcessClock::time_point cb::DefaultProcessClockSource::now() {
    return cb::ProcessClock::now();
}

static cb::DefaultProcessClockSource clockSource;

cb::DefaultProcessClockSource& cb::defaultProcessClockSource() {
    return clockSource;
}

std::chrono::nanoseconds cb::to_ns_since_epoch(
        const ProcessClock::time_point& tp) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                         tp.time_since_epoch());
}

std::string to_string(const std::chrono::nanoseconds& ns) {
    using namespace std::chrono;

    if (ns <= nanoseconds(9999)) {
        return std::to_string(ns.count()) + "ns";
    }

    if (ns <= microseconds(9999)) {
        auto us = duration_cast<microseconds>(ns);
        return std::to_string(us.count()) + "µs";
    }

    if (ns <= milliseconds(9999)) {
        auto ms = duration_cast<milliseconds>(ns);
        return std::to_string(ms.count()) + "ms";
    }

    if (ns <= seconds(599)) {
        auto s = duration_cast<seconds>(ns);
        return std::to_string(s.count()) + "s";
    }

    auto secs = duration_cast<seconds>(ns).count();
    int hour = static_cast<int>(secs / 3600);
    secs -= hour * 3600;
    int min = static_cast<int>(secs / 60);
    secs -= min * 60;
    return std::to_string(hour) + ":" + std::to_string(min) + ":" +
           std::to_string(secs);
}
