/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/byte_literals.h>

TEST(ByteLiteralsTest, KiB) {
    static_assert(std::is_same_v<decltype(1_KiB), std::size_t>);
    EXPECT_EQ(1_KiB, 1024);
}

TEST(ByteLiteralsTest, MiB) {
    static_assert(std::is_same_v<decltype(1_MiB), std::size_t>);
    EXPECT_EQ(1_MiB, 1024_KiB);
    EXPECT_EQ(1_MiB, 1024 * 1024);
}

TEST(ByteLiteralsTest, GiB) {
    static_assert(std::is_same_v<decltype(1_GiB), std::size_t>);
    EXPECT_EQ(1_GiB, 1024_MiB);
    EXPECT_EQ(1_GiB, 1024 * 1024 * 1024);
}

TEST(ByteLiteralsTest, TiB) {
    static_assert(std::is_same_v<decltype(1_TiB), std::size_t>);
    EXPECT_EQ(1_TiB, 1024_GiB);
    EXPECT_EQ(1_TiB, 1024ULL * 1024 * 1024 * 1024);
}
