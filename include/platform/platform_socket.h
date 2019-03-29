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
 * Header including C socket portability declarations
 */
#pragma once

#include <platform/dynamic.h>
#include <platform/visibility.h>

#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define CB_DONT_NEED_BYTEORDER 1
struct iovec {
    size_t iov_len;
    void* iov_base;
};

#define IOV_MAX 1024

struct msghdr {
    void* msg_name; /* Socket name */
    int msg_namelen; /* Length of name */
    struct iovec* msg_iov; /* Data blocks */
    int msg_iovlen; /* Number of blocks */
};

PLATFORM_PUBLIC_API
int sendmsg(SOCKET sock, const struct msghdr* msg, int flags);

/**
 * Initialize the winsock library
 */
PLATFORM_PUBLIC_API
void cb_initialize_sockets(void);
#else // WIN32
#define cb_initialize_sockets()
#endif // WIN32

#ifndef CB_DONT_NEED_BYTEORDER
PLATFORM_PUBLIC_API
uint64_t ntohll(uint64_t);

PLATFORM_PUBLIC_API
uint64_t htonll(uint64_t);
#endif

#ifdef __cplusplus
}
#endif
