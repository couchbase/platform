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

#ifndef CB_DONT_NEED_BYTEORDER
#include <folly/Bits.h>

inline uint64_t ntohll(uint64_t x) {
    return folly::Endian::big(x);
}

inline uint64_t htonll(uint64_t x) {
    return folly::Endian::big(x);
}
#endif // CB_DONT_NEED_BYTEORDER
