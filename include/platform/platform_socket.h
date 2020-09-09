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

#include <folly/portability/Sockets.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define CB_DONT_NEED_BYTEORDER 1

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

#ifdef __cplusplus
}
#endif

#ifndef CB_DONT_NEED_BYTEORDER
#include <folly/Bits.h>

PLATFORM_PUBLIC_API
inline uint64_t ntohll(uint64_t x) {
    return folly::Endian::big(x);
}

PLATFORM_PUBLIC_API
inline uint64_t htonll(uint64_t x) {
    return folly::Endian::big(x);
}
#endif // CB_DONT_NEED_BYTEORDER
