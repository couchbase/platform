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

#include <chrono>

#if defined(_MSC_VER) && _MSC_VER < 1900 /* less than Visual Studio 2015 */
/**
 * Following WIN32 implementation is heavily based on solution provided in
 * http://stackoverflow.com/questions/16299029/
 * resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme
 * and the potential overflow issue (see below) is addressed by adopting the
 * approach provided in Microsoft Visual Studio 2015.
 */

cb::SteadyClock::duration cb::SteadyClock::calculateDuration(
        const LONGLONG freq, const LARGE_INTEGER count) {
    /**
     * The implementation provided in the stackoverflow.com link returns the
     * result of the following calculation:
     * count.QuadPart * static_cast<rep>(period::den) / g_Frequency;
     *
     * It is possible for the count.QuadPart * static_cast<rep>(period::den)
     * calculation to overflow on a system that has been up for a long time.
     *
     * The solution provided by Microsoft in Visual Studio 2015 is to calculate
     * the whole and then the remainder and sum together.
     * Therefore we adopt the same approach below.
     */

    const LONGLONG whole = (count.QuadPart / freq) * period::den;
    const LONGLONG remainder = ((count.QuadPart % freq) * period::den) / freq;
    return duration(whole + remainder);
}

cb::SteadyClock::time_point cb::SteadyClock::now(void) {
    const LONGLONG g_Frequency = []() {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        return frequency.QuadPart;
    }();

    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);

    const auto dur = calculateDuration(g_Frequency, count);
    return time_point(dur);
}

#endif

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
