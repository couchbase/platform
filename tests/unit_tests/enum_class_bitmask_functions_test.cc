/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <platform/define_enum_class_bitmask_functions.h>

enum class Bitmask { None = 0, Two = 2, Four = 4, Eight = 8 };
DEFINE_ENUM_CLASS_BITMASK_FUNCTIONS(Bitmask);

TEST(Bitmask, Operators) {
    Bitmask bm = {};
    EXPECT_EQ(Bitmask::None, bm);
    bm |= Bitmask::Eight;
    EXPECT_EQ(Bitmask::Eight, bm);
    EXPECT_TRUE(isFlagSet(bm, Bitmask::Eight));
    EXPECT_FALSE(isFlagSet(bm, Bitmask::Four));

    bm |= Bitmask::Four;
    EXPECT_TRUE(isFlagSet(bm, Bitmask::Eight));
    EXPECT_TRUE(isFlagSet(bm, Bitmask::Four));
    EXPECT_TRUE(isFlagSet(bm, Bitmask::Eight | Bitmask::Four));
    EXPECT_FALSE(isFlagSet(bm, Bitmask::Eight | Bitmask::Four | Bitmask::Two));

    bm &= ~Bitmask::Four;
    EXPECT_TRUE(isFlagSet(bm, Bitmask::Eight));
    EXPECT_FALSE(isFlagSet(bm, Bitmask::Four));
    EXPECT_EQ(Bitmask::Eight, bm);
}
