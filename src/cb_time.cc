/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#elif defined(__linux__) || defined(__sun) || defined(__FreeBSD__)
    /* Linux and Solaris can use clock_gettime */
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
    int rv = gettimeofday(tv, NULL);
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
    return gmtime_r(clock, result) == NULL ? -1 : 0;
#endif
}

int cb_localtime_r(const time_t *clock, struct tm *result)
{
#ifdef WIN32
    return localtime_s(result, clock) == 0 ? 0 : -1;
#else
    return localtime_r(clock, result) == NULL ? -1 : 0;
#endif
}
