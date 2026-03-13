/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>

#include <platform/roundrobin.h>

#include <string>
#include <vector>

using namespace cb;

// ---------------------------------------------------------------------------
// RoundRobin - Construction / capacity
// ---------------------------------------------------------------------------
TEST(RoundRobin, ConstructWithEmptyVector) {
    std::vector<int> vec;
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_EQ(0u, rr.size());
}

TEST(RoundRobin, ConstructWithSingleElement) {
    std::vector<int> vec = {42};
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_EQ(1u, rr.size());
}

TEST(RoundRobin, ConstructWithMultipleElements) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_EQ(5u, rr.size());
}

// ---------------------------------------------------------------------------
// RoundRobin - Basic iteration behavior
// ---------------------------------------------------------------------------
TEST(RoundRobin, NextReturnsElementsInOrder) {
    std::vector<int> vec = {1, 2, 3};
    RoundRobin<std::vector<int>> rr(vec);

    EXPECT_EQ(1, rr.next());
    EXPECT_EQ(2, rr.next());
    EXPECT_EQ(3, rr.next());
}

TEST(RoundRobin, NextWrapsAroundToBeginning) {
    std::vector<int> vec = {10, 20, 30};
    RoundRobin<std::vector<int>> rr(vec);

    EXPECT_EQ(10, rr.next());
    EXPECT_EQ(20, rr.next());
    EXPECT_EQ(30, rr.next());
    EXPECT_EQ(10, rr.next());
    EXPECT_EQ(20, rr.next());
}

TEST(RoundRobin, NextReturnsReferenceToOriginalData) {
    std::vector<int> vec = {1, 2, 3};
    RoundRobin<std::vector<int>> rr(vec);

    int& ref = rr.next();
    ref = 100;

    EXPECT_EQ(100, vec[0]);
}

// ---------------------------------------------------------------------------
// RoundRobin - Round-robin cycling
// ---------------------------------------------------------------------------
TEST(RoundRobin, MultipleCyclesWithoutGaps) {
    std::vector<int> vec = {1, 2};
    RoundRobin<std::vector<int>> rr(vec);

    for (int cycle = 0; cycle < 5; ++cycle) {
        EXPECT_EQ(1, rr.next());
        EXPECT_EQ(2, rr.next());
    }
}

TEST(RoundRobin, CyclingThroughManyElements) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    RoundRobin<std::vector<int>> rr(vec);

    for (int i = 0; i < 15; ++i) {
        EXPECT_EQ(vec[i % 5], rr.next());
    }
}

TEST(RoundRobin, ConstNextWorksWithConstRoundRobin) {
    std::vector<int> vec = {100, 200, 300};
    const RoundRobin<std::vector<int>> rr(vec);

    EXPECT_EQ(100, rr.next());
    EXPECT_EQ(200, rr.next());
    EXPECT_EQ(300, rr.next());
    EXPECT_EQ(100, rr.next());
}

// ---------------------------------------------------------------------------
// RoundRobin - Edge cases
// ---------------------------------------------------------------------------
TEST(RoundRobin, EmptyContainerReturnsTrueFromEmpty) {
    std::vector<int> vec;
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_TRUE(rr.empty());
}

TEST(RoundRobin, NonEmptyContainerReturnsFalseFromEmpty) {
    std::vector<int> vec = {42};
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_FALSE(rr.empty());
}

TEST(RoundRobin, NextThrowsOnEmptyContainer) {
    std::vector<int> vec;
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_THROW((void)rr.next(), std::runtime_error);
}

TEST(RoundRobin, NextThrowsOnEmptyContainerConst) {
    std::vector<int> vec;
    const RoundRobin<std::vector<int>> rr(vec);
    EXPECT_THROW((void)rr.next(), std::runtime_error);
}

TEST(RoundRobin, SingleElementAlwaysReturnsSame) {
    std::vector<int> vec = {42};
    RoundRobin<std::vector<int>> rr(vec);

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(42, rr.next());
    }
}

TEST(RoundRobin, WorksWithStdString) {
    std::vector<std::string> vec = {"hello", "world", "foo"};
    RoundRobin<std::vector<std::string>> rr(vec);

    EXPECT_EQ("hello", rr.next());
    EXPECT_EQ("world", rr.next());
    EXPECT_EQ("foo", rr.next());
    EXPECT_EQ("hello", rr.next());
}

TEST(RoundRobin, ReturnsReferenceToStdString) {
    std::vector<std::string> vec = {"test"};
    RoundRobin<std::vector<std::string>> rr(vec);

    std::string& ref = rr.next();
    ref = "modified";

    EXPECT_EQ("modified", vec[0]);
}

TEST(RoundRobin, WorksWithPointerElements) {
    std::vector<int*> vec = {new int(1), new int(2), new int(3)};
    RoundRobin<std::vector<int*>> rr(vec);

    EXPECT_EQ(1, *rr.next());
    EXPECT_EQ(2, *rr.next());
    EXPECT_EQ(3, *rr.next());
    EXPECT_EQ(1, *rr.next());

    for (auto ptr : vec) {
        delete ptr;
    }
}

// ---------------------------------------------------------------------------
// RoundRobin - Array support
// ---------------------------------------------------------------------------
TEST(RoundRobin, ConstructWithStdArray) {
    std::array<int, 4> arr = {1, 2, 3, 4};
    RoundRobin<std::array<int, 4>> rr(arr);

    EXPECT_EQ(4u, rr.size());
    EXPECT_EQ(1, rr.next());
    EXPECT_EQ(2, rr.next());
    EXPECT_EQ(3, rr.next());
    EXPECT_EQ(4, rr.next());
}

// ---------------------------------------------------------------------------
// RoundRobin - Size queries
// ---------------------------------------------------------------------------
TEST(RoundRobin, SizeMatchesVectorLength) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    RoundRobin<std::vector<int>> rr(vec);
    EXPECT_EQ(5u, rr.size());
}

// ---------------------------------------------------------------------------
// RoundRobin - Multiple instances independent
// ---------------------------------------------------------------------------
TEST(RoundRobin, MultipleInstancesHaveIndependentState) {
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<int> vec2 = {10, 20};

    RoundRobin<std::vector<int>> rr1(vec1);
    RoundRobin<std::vector<int>> rr2(vec2);

    EXPECT_EQ(1, rr1.next());
    EXPECT_EQ(10, rr2.next());
    EXPECT_EQ(2, rr1.next());
    EXPECT_EQ(20, rr2.next());
    EXPECT_EQ(3, rr1.next());
    EXPECT_EQ(10, rr2.next());
}
