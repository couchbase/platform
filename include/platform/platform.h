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

#include <platform/dynamic.h>
#include <platform/visibility.h>
#include <platform/cbassert.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef __sun
#include <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
    typedef long ssize_t;
#define CB_DONT_NEED_BYTEORDER 1
#define DIRECTORY_SEPARATOR_CHARACTER '\\'

#else

#define DIRECTORY_SEPARATOR_CHARACTER '/'

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
#else

#define cb_initialize_sockets()

#endif

    /*
     * Set mode to binary
     */
    PLATFORM_PUBLIC_API
    int platform_set_binary_mode(FILE *fp);

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

#ifdef __cplusplus
}
#endif
