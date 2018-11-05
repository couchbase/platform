/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc.
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

/*
 * Header including time portability declarations
 */
#pragma once

#include <platform/visibility.h>

#include <stdint.h>
#include <time.h>

#include <folly/portability/SysTime.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
typedef unsigned __int64 hrtime_t;
#else
#include <sys/time.h>
#ifndef __sun
typedef uint64_t hrtime_t;
#endif // __sun
#endif // WIN32

/**
 * return a monotonically increasing value with a seconds frequency.
 */
PLATFORM_PUBLIC_API
uint64_t cb_get_monotonic_seconds(void);

/**
 * obtain a timeval structure containing the current time since EPOCH.
 */
PLATFORM_PUBLIC_API
int cb_get_timeofday(struct timeval* tv);

/**
 * Set an offset (in seconds) added to cb_get_timeofday before returned
 * to the caller.
 *
 * This is intended for testing of time jumps.
 *
 * @param offset the number of seconds to add (a negative value results in
 *               jumping back in time)
 */
PLATFORM_PUBLIC_API
void cb_set_timeofday_offset(int offset);

/**
 * Get the offset being added to the cb_get_timeofday()
 */
PLATFORM_PUBLIC_API
int cb_get_timeofday_offset(void);

/**
 * Set an uptime offset to be added to memcached_uptime
 *
 * This is intended for the testing of expiry.
 *
 * @param offset the number of seconds to add
 *
 */
PLATFORM_PUBLIC_API
void cb_set_uptime_offset(uint64_t offset);

/**
 * Get the offset to add to the uptime.
 */
PLATFORM_PUBLIC_API
uint64_t cb_get_uptime_offset();

/**
 * Travel in time by updating the timeofday_offset with a relative
 * value
 *
 * @param secs the number of seconds to travel
 */
PLATFORM_PUBLIC_API
void cb_timeofday_timetravel(int offset);

/**
 * Convert time_t to a structure
 *
 * @param clock the input value
 * @param result the output value
 * @return 0 for success, -1 on failure
 */
PLATFORM_PUBLIC_API
int cb_gmtime_r(const time_t* clock, struct tm* result);

/**
 * Convert a time value with adjustments for the local time zone
 *
 * @param clock the input value
 * @param result the output value
 * @return 0 for success, -1 on failure
 */
PLATFORM_PUBLIC_API
int cb_localtime_r(const time_t* clock, struct tm* result);

#ifdef __cplusplus
}
#endif
