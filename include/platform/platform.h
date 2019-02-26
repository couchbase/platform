/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2013 Couchbase, Inc
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

#include <stdbool.h>

#include <platform/dynamic.h>
#include <platform/visibility.h>
#include <platform/cbassert.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include <time.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __sun
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef long ssize_t;
    typedef unsigned __int64 hrtime_t;
#define CB_DONT_NEED_BYTEORDER 1
#define DIRECTORY_SEPARATOR_CHARACTER '\\'

#else

#define DIRECTORY_SEPARATOR_CHARACTER '/'

#ifndef __sun
    typedef uint64_t hrtime_t;
#endif

#endif

#ifndef CB_DONT_NEED_BYTEORDER
    PLATFORM_PUBLIC_API
    uint64_t ntohll(uint64_t);

    PLATFORM_PUBLIC_API
    uint64_t htonll(uint64_t);
#endif

#ifdef WIN32
    struct iovec {
        size_t iov_len;
        void *iov_base;
    };

#define IOV_MAX 1024

    struct msghdr {
        void *msg_name;         /* Socket name */
        int msg_namelen;       /* Length of name */
        struct iovec *msg_iov; /* Data blocks */
        int msg_iovlen;        /* Number of blocks */
    };

    PLATFORM_PUBLIC_API
    int sendmsg(SOCKET sock, const struct msghdr *msg, int flags);

    /**
     * Initialize the winsock library
     */
    PLATFORM_PUBLIC_API
    void cb_initialize_sockets(void);

    PLATFORM_PUBLIC_API
    int gettimeofday(struct timeval *tv, void *tz);
#else

#define cb_initialize_sockets()

#endif

    /*
     * Set mode to binary
     */
    PLATFORM_PUBLIC_API
    int platform_set_binary_mode(FILE *fp);

    /*
        return a monotonically increasing value with a seconds frequency.
    */
    PLATFORM_PUBLIC_API
    uint64_t cb_get_monotonic_seconds(void);

    /*
        obtain a timeval structure containing the current time since EPOCH.
    */
    PLATFORM_PUBLIC_API
    int cb_get_timeofday(struct timeval *tv);

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
     * Some of our platforms complain on not using mkstemp. Instead of
     * having the test in all programs we're just going to use this
     * method instead.
     *
     * @param pattern The input pattern for the filename. It should end
     *                with six X's that will be replaced with the unique
     *                filename. The file will be created.
     * @return pattern on success, NULL upon failure. Check errno for
     *                 the reason.
     */
    PLATFORM_PUBLIC_API
    char *cb_mktemp(char *pattern);

    /**
     * Convert time_t to a structure
     *
     * @param clock the input value
     * @param result the output value
     * @return 0 for success, -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_gmtime_r(const time_t *clock, struct tm *result);


    /**
     * Convert a time value with adjustments for the local time zone
     *
     * @param clock the input value
     * @param result the output value
     * @return 0 for success, -1 on failure
     */
    PLATFORM_PUBLIC_API
    int cb_localtime_r(const time_t *clock, struct tm *result);

#ifdef __cplusplus
}
#endif
