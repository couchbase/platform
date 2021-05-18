/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/socket.h>

TEST(CbSocket, getIpAddresses_all) {
    auto [ipv4, ipv6] = cb::net::getIpAddresses(false);
    if (!ipv4.empty()) {
        EXPECT_NE(ipv4.end(), std::find(ipv4.begin(), ipv4.end(), "127.0.0.1"));
    }
    if (!ipv6.empty()) {
        EXPECT_NE(ipv6.end(), std::find(ipv6.begin(), ipv6.end(), "::1"));
    }
}

TEST(CbSocket, getIpAddresses_skipLocal) {
    auto [ipv4, ipv6] = cb::net::getIpAddresses(true);
    if (!ipv4.empty()) {
        EXPECT_EQ(ipv4.end(), std::find(ipv4.begin(), ipv4.end(), "127.0.0.1"));
    }
    if (!ipv6.empty()) {
        EXPECT_EQ(ipv6.end(), std::find(ipv6.begin(), ipv6.end(), "::1"));
    }
}
