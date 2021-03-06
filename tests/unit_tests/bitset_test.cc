/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/bitset.h>

enum TestStates { s1, s2, s3, s4, s5, s6, s7, end };

using TestStatesSet = cb::bitset<end, TestStates>;

enum class TestStates2 { s1, s2, s3, s4, s5, s6, s7, end };

using TestStates2Set = cb::bitset<size_t(TestStates2::end), TestStates2>;

TEST(BitsetTest, test0) {
    TestStatesSet test;
    EXPECT_FALSE(test.test(s1));
    EXPECT_FALSE(test.test(s2));
    EXPECT_FALSE(test.test(s3));
    EXPECT_FALSE(test.test(s4));
    EXPECT_FALSE(test.test(s5));
    EXPECT_FALSE(test.test(s6));
    EXPECT_FALSE(test.test(s7));
}

TEST(BitsetTest, test1) {
    TestStatesSet test(s1, s2, s3);
    EXPECT_TRUE(test.test(s1));
    EXPECT_TRUE(test.test(s2));
    EXPECT_TRUE(test.test(s3));
    EXPECT_FALSE(test.test(s4));
    EXPECT_FALSE(test.test(s5));
    EXPECT_FALSE(test.test(s6));
    EXPECT_FALSE(test.test(s7));

    test.set(s5);
    EXPECT_TRUE(test.test(s5));

    test.reset(s1);
    EXPECT_FALSE(test.test(s1));
}

TEST(BitsetTest, test2) {
    TestStates2Set test(TestStates2::s1, TestStates2::s2, TestStates2::s3);
    EXPECT_TRUE(test.test(TestStates2::s1));
    EXPECT_TRUE(test.test(TestStates2::s2));
    EXPECT_TRUE(test.test(TestStates2::s3));
    EXPECT_FALSE(test.test(TestStates2::s4));
    EXPECT_FALSE(test.test(TestStates2::s5));
    EXPECT_FALSE(test.test(TestStates2::s6));
    EXPECT_FALSE(test.test(TestStates2::s7));
}

enum TestStates3 { ts1 = 1, ts2, ts3, ts4, ts5, ts6, ts7, tsend };

struct TestStates3Map {
    [[nodiscard]] size_t map(TestStates3 in) const {
        return in - 1;
    }
};

using TestStates3Set = cb::bitset<7, TestStates3, TestStates3Map>;

TEST(BitsetTest, test3) {
    TestStates3Set test(ts1, ts2, ts3);
    EXPECT_TRUE(test.test(ts1));
    EXPECT_TRUE(test.test(ts2));
    EXPECT_TRUE(test.test(ts3));
    EXPECT_FALSE(test.test(ts4));
    EXPECT_FALSE(test.test(ts5));
    EXPECT_FALSE(test.test(ts6));
    EXPECT_FALSE(test.test(ts7));
}

enum Messy { m1 = 4, m2 = 55, m3 = 19, m4 = 102 };

struct MessyMap {
    [[nodiscard]] size_t map(Messy in) const {
        switch (in) {
        case m1:
            return 0;
        case m2:
            return 1;
        case m3:
            return 2;
        case m4:
            return 3;
        default:
            throw std::invalid_argument("MessyMap::map " + std::to_string(in));
        }
    }
};

using MessySet = cb::bitset<4, Messy, MessyMap>;
TEST(BitsetTest, messyMap) {
    MessySet test(m4, m2);
    EXPECT_TRUE(test.test(m2));
    EXPECT_TRUE(test.test(m4));
    EXPECT_FALSE(test.test(m1));
    EXPECT_FALSE(test.test(m3));
}
