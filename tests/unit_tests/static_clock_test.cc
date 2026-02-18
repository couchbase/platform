/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/cb_time.h>

#include <folly/ScopeGuard.h>
#include <folly/portability/GTest.h>

TEST(StaticClockTest, test) {
    cb::time::StaticClockGuard guard{};
    auto time1 = cb::time::steady_clock::now();
    auto time2 = cb::time::steady_clock::now();
    EXPECT_EQ(time1, time2);
    using namespace std::chrono_literals;
    cb::time::steady_clock::advance(1ns);
    auto time3 = cb::time::steady_clock::now();
    EXPECT_GT(time3, time2);
}
