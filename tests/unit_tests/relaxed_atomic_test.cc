/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <relaxed_atomic.h>

#include <folly/portability/GTest.h>

// Test that we can use RelaxedAtomic<T> in STL containers.
TEST(RelaxedAtomicTest, STLContainer) {
    std::vector<cb::RelaxedAtomic<uint64_t>> vec;

    // Check we can populate a std::vector with assign()
    vec.assign(3, 1);
    EXPECT_EQ(1u, vec[0]);
    EXPECT_EQ(1u, vec[1]);
    EXPECT_EQ(1u, vec[2]);

    // Check we can change existing values
    vec[2] = 2;
    EXPECT_EQ(2u, vec[2]);
}

TEST(RelaxedAtomicTest, setIfSmaller) {
    cb::RelaxedAtomic<uint8_t> val;
    val.store(10);

    // Check we don't store larger numbers
    val.setIfSmaller(15);
    EXPECT_EQ(10u, val.load());

    // Check we store smaller numbers
    val.setIfSmaller(5);
    EXPECT_EQ(5u, val.load());

    cb::RelaxedAtomic<uint8_t> smaller;
    smaller.store(3);

    // Check we can correctly store from another cb::RelaxedAtomic
    val.setIfSmaller(smaller);
    EXPECT_EQ(3u, val.load());
}

TEST(RelaxedAtomicTest, setAdd) {
    cb::RelaxedAtomic<uint8_t> val;
    val.store(5);

    // Check we can add to the value
    val.setAdd(10);
    EXPECT_EQ(15u, val.load());

    cb::RelaxedAtomic<uint8_t> add;
    add.store(5);

    // Check we can add from another cb::RelaxedAtomic
    val.setAdd(add);
    EXPECT_EQ(20u, val.load());
}

TEST(RelaxedAtomicTest, setSub) {
    cb::RelaxedAtomic<uint8_t> val;
    val.store(10);

    // Check we can subtract from the value
    val.setSub(5);
    EXPECT_EQ(5u, val.load());

    cb::RelaxedAtomic<uint8_t> sub;
    sub.store(2);

    // Check we can subtract from the value from another
    // cb::RelaxedAtomic
    val.setSub(sub);
    EXPECT_EQ(3u, val.load());
}
