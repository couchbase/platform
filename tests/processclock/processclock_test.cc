/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <thread>

#include <folly/portability/GTest.h>
#include <platform/processclock.h>

TEST(DefaultProcessClockSourceTest, SensibleBounds) {
    auto a = std::chrono::steady_clock::now();
    auto b = cb::defaultProcessClockSource().now();
    auto c = std::chrono::steady_clock::now();

    EXPECT_LE(a, b);
    EXPECT_LE(b, c);
}