/*
 *     Copyright 2020 Couchbase, Inc.
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
