/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/bifurcated_counter.h>
#include <relaxed_atomic.h>

#include <folly/portability/GTest.h>

template <typename Value>
class BifurcatedCounterTest : public ::testing::Test {};

using TestTypes =
        ::testing::Types<int, long, std::atomic<int>, cb::RelaxedAtomic<int>>;
class TestTypeNames {
public:
    template <typename T>
    static std::string GetName(int) {
        if (std::is_same<T, int>())
            return "int";
        if (std::is_same<T, long>())
            return "long";
        if (std::is_same<T, std::atomic<int>>())
            return "atomic_int";
        if (std::is_same<T, cb::RelaxedAtomic<int>>())
            return "relaxed_atomic_int";
        throw std::runtime_error("Unreachable");
    }
};
TYPED_TEST_SUITE(BifurcatedCounterTest, TestTypes, TestTypeNames);

TYPED_TEST(BifurcatedCounterTest, InitialValue) {
    cb::BifurcatedCounter<TypeParam> x;
    EXPECT_EQ(0, x.load());
    EXPECT_EQ(0, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, Addition) {
    cb::BifurcatedCounter<TypeParam> x;
    x += 1;
    EXPECT_EQ(1, x.load());
    EXPECT_EQ(1, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, AdditionOfNegative) {
    cb::BifurcatedCounter<TypeParam> x;
    x += -1;
    EXPECT_EQ(-1, x.load());
    EXPECT_EQ(0, x.getAdded());
    EXPECT_EQ(1, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, Subtraction) {
    cb::BifurcatedCounter<TypeParam> x;
    x -= 1;
    EXPECT_EQ(-1, x.load());
    EXPECT_EQ(0, x.getAdded());
    EXPECT_EQ(1, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, SubtractionOfNegative) {
    cb::BifurcatedCounter<TypeParam> x;
    x -= -1;
    EXPECT_EQ(1, x.load());
    EXPECT_EQ(1, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, Increments) {
    cb::BifurcatedCounter<TypeParam> x;

    x++;
    EXPECT_EQ(1, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());

    ++x;
    EXPECT_EQ(2, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());

    x--;
    EXPECT_EQ(2, x.getAdded());
    EXPECT_EQ(1, x.getRemoved());

    --x;
    EXPECT_EQ(2, x.getAdded());
    EXPECT_EQ(2, x.getRemoved());
}

TYPED_TEST(BifurcatedCounterTest, Reset) {
    cb::BifurcatedCounter<TypeParam> x;
    x += 10'000;
    x -= 1'000;
    x += 100;
    x -= 10;
    x += -1;

    ASSERT_EQ(10'000 - 1'000 + 100 - 10 - 1, x.load());
    ASSERT_EQ(10'000 + 100, x.getAdded());
    ASSERT_EQ(1'000 + 10 + 1, x.getRemoved());

    x.reset();

    EXPECT_EQ(0, x.load());
    EXPECT_EQ(0, x.getAdded());
    EXPECT_EQ(0, x.getRemoved());
}
