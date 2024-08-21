/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/checked_snprintf.h>
#include <array>

TEST(checked_snprintf, DestinationNullptr) {
    EXPECT_THROW(checked_snprintf(nullptr, 10, "xyz"), std::invalid_argument);
}

TEST(checked_snprintf, DestinationSize0) {
    std::array<char,20> buf{};
    EXPECT_THROW(checked_snprintf(buf.data(), 0, "xyz"), std::invalid_argument);
}

TEST(checked_snprintf, FitInBuffer) {
    char buffer[10];
    EXPECT_EQ(4, checked_snprintf(buffer, sizeof(buffer), "test"));
    EXPECT_EQ(std::string("test"), buffer);
}

TEST(checked_snprintf, BufferTooSmall) {
    char buffer[10];
    EXPECT_THROW(checked_snprintf(buffer, sizeof(buffer), "test %s %u",
                                  "with a buffer that is too big", 10),
                 std::overflow_error);
}
