/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

#include <platform/platform.h>

#include <chrono>

namespace cb {

/**
 * ProcessClock provides an interface to std::chrono::steady_clock.
 * The steady_clock is not related to wall clock time and cannot be decreased.
 * This is useful when we want a task to run at a set interval regardless if
 * the system wall clock is changed.
 */

#if defined(_MSC_VER) && _MSC_VER < 1900 /* less than Visual Studio 2015 */
/**
 * Due to a defect in Microsoft's implementation of the steady_clock
 * it does not have sufficiently high resolution.
 * See the following link for more details
 * https://web.archive.org/web/20141212192132/https://connect.microsoft.com/
 * VisualStudio/feedback/details/719443/
 *
 * The defect has been fixed in Visual Studio 2015, however we are currently
 * compiling using Visual Studio 2013 or earlier.  Therefore we need to provide
 * our own implementation of the steady_clock.
 */

struct PLATFORM_PUBLIC_API SteadyClock {
    typedef long long rep;
    typedef std::nano period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<SteadyClock>   time_point;
    static const bool is_steady = true;
    static time_point now(void);

protected:
    static SteadyClock::duration calculateDuration(const LONGLONG freq,
                                                   const LARGE_INTEGER count);
};

// Using ProcessClock to hide the differences in implementation between Windows
// and non-Windows
using ProcessClock = SteadyClock;

#else

// For non-Windows we just use the std::chrono::steady_clock
using ProcessClock = std::chrono::steady_clock;

#endif

// Simple wrapper function that returns std::chrono::nanoseconds
// given a ProcessClock::time_point
PLATFORM_PUBLIC_API
std::chrono::nanoseconds to_ns_since_epoch(const ProcessClock::time_point& tp);

/**
 * Interface for a source of 'now' for the ProcessClock to allow
 * for dependency injection of time.
 */
struct PLATFORM_PUBLIC_API ProcessClockSource {
    virtual ProcessClock::time_point now() = 0;
};

/**
 * Default 'now' source for ProcessClock, simply proxies ProcessClock::now()
 */
struct PLATFORM_PUBLIC_API DefaultProcessClockSource : ProcessClockSource {
    ProcessClock::time_point now() override;
};

/**
 * Singleton instance of DefaultProcessClockSource
 */
PLATFORM_PUBLIC_API
DefaultProcessClockSource& defaultProcessClockSource();
}

// Import ProcessClock and to_ns_since_epoch into global namespace
using ProcessClock = cb::ProcessClock;

PLATFORM_PUBLIC_API
inline std::chrono::nanoseconds to_ns_since_epoch(
        const ProcessClock::time_point& tp) {
    return cb::to_ns_since_epoch(tp);
}
