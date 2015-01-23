/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc
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

#include "config.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if defined(WIN32)
#include <Windows.h>
#endif

#include <platform/platform.h>

static uint64_t timeofday_offset = 0;

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
#elif defined(__linux__) || defined(__sun)
    /* Linux and Solaris can use clock_gettime */
    struct timespec tm;
    if (clock_gettime(CLOCK_MONOTONIC, &tm) == -1) {
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
#ifdef WIN32
    tv->tv_sec += (long)timeofday_offset;
#else
    tv->tv_sec += timeofday_offset;
#endif
    return rv;
}

/*
    set an offset so that cb_get_timeofday returns an offsetted time.
    This is intended for testing of time jumps.
*/
void cb_set_timeofday_offset(uint64_t offset) {
    timeofday_offset = offset;
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
