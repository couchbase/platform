/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
 * Header including time portability declarations
 */
#pragma once


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
typedef uint64_t hrtime_t;
#endif // WIN32

/**
 * return a monotonically increasing value with a seconds frequency.
 */
uint64_t cb_get_monotonic_seconds(void);

/**
 * obtain a timeval structure containing the current time since EPOCH.
 */
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
void cb_set_timeofday_offset(int offset);

/**
 * Get the offset being added to the cb_get_timeofday()
 */
int cb_get_timeofday_offset(void);

/**
 * Set an uptime offset to be added to memcached_uptime
 *
 * This is intended for the testing of expiry.
 *
 * @param offset the number of seconds to add
 *
 */
void cb_set_uptime_offset(uint64_t offset);

/**
 * Get the offset to add to the uptime.
 */
uint64_t cb_get_uptime_offset();

/**
 * Travel in time by updating the timeofday_offset with a relative
 * value
 *
 * @param secs the number of seconds to travel
 */
void cb_timeofday_timetravel(int offset);

/**
 * Convert time_t to a structure
 *
 * @param clock the input value
 * @param result the output value
 * @return 0 for success, -1 on failure
 */
int cb_gmtime_r(const time_t* clock, struct tm* result);

/**
 * Convert a time value with adjustments for the local time zone
 *
 * @param clock the input value
 * @param result the output value
 * @return 0 for success, -1 on failure
 */
int cb_localtime_r(const time_t* clock, struct tm* result);

#ifdef __cplusplus
}
#endif
