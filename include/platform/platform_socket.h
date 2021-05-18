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
 * Header including C socket portability declarations
 */
#pragma once

#include <platform/dynamic.h>

#include <folly/portability/Sockets.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define CB_DONT_NEED_BYTEORDER 1

int sendmsg(SOCKET sock, const struct msghdr* msg, int flags);

/**
 * Initialize the winsock library
 */
void cb_initialize_sockets(void);
#else // WIN32
#define cb_initialize_sockets()
#endif // WIN32

#ifdef __cplusplus
}
#endif

#ifndef CB_DONT_NEED_BYTEORDER
#include <folly/Bits.h>

inline uint64_t ntohll(uint64_t x) {
    return folly::Endian::big(x);
}

inline uint64_t htonll(uint64_t x) {
    return folly::Endian::big(x);
}
#endif // CB_DONT_NEED_BYTEORDER
