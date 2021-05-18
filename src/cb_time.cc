/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/platform_time.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if defined(WIN32)
#include <folly/portability/Windows.h>
#endif

static std::atomic_int timeofday_offset { 0 };
static std::atomic_uint uptime_offset{0};

/*
    return a monotonically increasing value with a seconds frequency.
*/
uint64_t cb_get_monotonic_seconds() {
    uint64_t seconds = 0;
#if defined(WIN32)
    /* GetTickCound64 gives us near 60years of ticks...*/
    seconds =  (GetTickCount64() / 1000);
#elif defined(__APPLE__)
    uint64_t time = mach_absolute_time();

    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
      mach_timebase_info(&timebase);
    }

    seconds = (double)time * timebase.numer / timebase.denom * 1e-9;
#elif defined(__linux__)
    /* Linux can use clock_gettime */
    struct timespec tm;
    if (clock_gettime(CLOCK_MONOTONIC, &tm) == -1) {
        fprintf(stderr, "clock_gettime failed, aborting program: %s",
                strerror(errno));
        fflush(stderr);
        abort();
    }
    seconds = tm.tv_sec;
#else
#error "Don't know how to build cb_get_monotonic_seconds"
#endif

    return seconds;
}

/*
    obtain a timeval structure containing the current time since EPOCH.
*/
int cb_get_timeofday(struct timeval *tv) {
    int rv = gettimeofday(tv, nullptr);
    tv->tv_sec += timeofday_offset.load(std::memory_order_relaxed);
    return rv;
}

void cb_set_timeofday_offset(int offset) {
    timeofday_offset.store(offset, std::memory_order_relaxed);
}

int cb_get_timeofday_offset(void) {
    return timeofday_offset.load(std::memory_order_relaxed);
}

void cb_set_uptime_offset(uint64_t offset) {
    uptime_offset.store(offset, std::memory_order_relaxed);
}

uint64_t cb_get_uptime_offset(void) {
    return uptime_offset.load(std::memory_order_relaxed);
}

void cb_timeofday_timetravel(int offset) {
    timeofday_offset.fetch_add(offset, std::memory_order_relaxed);
}

int cb_gmtime_r(const time_t *clock, struct tm *result)
{
#ifdef WIN32
    return gmtime_s(result, clock) == 0 ? 0 : -1;
#else
    return gmtime_r(clock, result) == nullptr ? -1 : 0;
#endif
}

int cb_localtime_r(const time_t *clock, struct tm *result)
{
#ifdef WIN32
    return localtime_s(result, clock) == 0 ? 0 : -1;
#else
    return localtime_r(clock, result) == nullptr ? -1 : 0;
#endif
}
