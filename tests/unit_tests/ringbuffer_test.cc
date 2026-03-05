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

#include <platform/ringbuffer.h>

#include <string>
#include <vector>

using namespace cb;

// ---------------------------------------------------------------------------
// Construction / capacity
// ---------------------------------------------------------------------------
TEST(RingBuffer, ConstructWithNoCapacity) {
    RingBuffer<int> buf(0);
    EXPECT_EQ(0u, buf.capacity());
    EXPECT_EQ(0u, buf.size());
    EXPECT_TRUE(buf.is_empty());
    EXPECT_TRUE(buf.is_full());
    buf.push(1);
    EXPECT_EQ(0u, buf.size());
}

TEST(RingBuffer, ConstructWithCapacityOne) {
    RingBuffer<int> buf(1);
    EXPECT_EQ(1u, buf.capacity());
    EXPECT_EQ(0u, buf.size());
    EXPECT_TRUE(buf.is_empty());
    EXPECT_FALSE(buf.is_full());
}

TEST(RingBuffer, ConstructWithLargerCapacity) {
    RingBuffer<int> buf(10);
    EXPECT_EQ(10u, buf.capacity());
    EXPECT_EQ(0u, buf.size());
    EXPECT_TRUE(buf.is_empty());
    EXPECT_FALSE(buf.is_full());
}

// ---------------------------------------------------------------------------
// is_empty / is_full / size
// ---------------------------------------------------------------------------

TEST(RingBuffer, IsEmptyAfterConstruction) {
    RingBuffer<int> buf(5);
    EXPECT_TRUE(buf.is_empty());
    EXPECT_FALSE(buf.is_full());
    EXPECT_EQ(0u, buf.size());
}

TEST(RingBuffer, IsFullAfterFillingToCapacity) {
    RingBuffer<int> buf(3);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    EXPECT_TRUE(buf.is_full());
    EXPECT_FALSE(buf.is_empty());
    EXPECT_EQ(3u, buf.size());
}

TEST(RingBuffer, PartiallyFilledIsNeitherEmptyNorFull) {
    RingBuffer<int> buf(5);
    buf.push(1);
    buf.push(2);
    EXPECT_FALSE(buf.is_empty());
    EXPECT_FALSE(buf.is_full());
    EXPECT_EQ(2u, buf.size());
}

TEST(RingBuffer, SizeGrowsWithEachPush) {
    RingBuffer<int> buf(4);
    for (std::size_t i = 1; i <= 4; ++i) {
        buf.push(static_cast<int>(i));
        EXPECT_EQ(i, buf.size());
    }
}

TEST(RingBuffer, SizeDoesNotExceedCapacity) {
    RingBuffer<int> buf(3);
    for (int i = 0; i < 10; ++i) {
        buf.push(i);
        EXPECT_LE(buf.size(), 3u);
    }
    EXPECT_EQ(3u, buf.size());
}

// ---------------------------------------------------------------------------
// push + iterate: basic ordering
// ---------------------------------------------------------------------------

TEST(RingBuffer, IterateEmptyBuffer) {
    RingBuffer<int> buf(5);
    int count = 0;
    buf.iterate([&](const int&) { ++count; });
    EXPECT_EQ(0, count);
}

TEST(RingBuffer, IteratePartiallyFilledBuffer) {
    RingBuffer<int> buf(5);
    buf.push(10);
    buf.push(20);
    buf.push(30);

    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(10, result[0]);
    EXPECT_EQ(20, result[1]);
    EXPECT_EQ(30, result[2]);
}

TEST(RingBuffer, IterateFullBuffer) {
    RingBuffer<int> buf(4);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.push(4);

    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(4u, result.size());
    EXPECT_EQ(1, result[0]);
    EXPECT_EQ(2, result[1]);
    EXPECT_EQ(3, result[2]);
    EXPECT_EQ(4, result[3]);
}

// ---------------------------------------------------------------------------
// Overwrite behaviour (ring wrap-around)
// ---------------------------------------------------------------------------

TEST(RingBuffer, OverwriteDropsOldestElement) {
    RingBuffer<int> buf(3);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    // Buffer is full: [1, 2, 3]
    buf.push(4);
    // Oldest (1) is overwritten: [2, 3, 4]

    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(2, result[0]);
    EXPECT_EQ(3, result[1]);
    EXPECT_EQ(4, result[2]);
}

TEST(RingBuffer, MultipleOverwritesPreserveOrder) {
    RingBuffer<int> buf(3);
    for (int i = 1; i <= 7; ++i) {
        buf.push(i);
    }
    // After pushing 1..7 into capacity-3 buffer the last 3 survive: [5, 6, 7]
    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(5, result[0]);
    EXPECT_EQ(6, result[1]);
    EXPECT_EQ(7, result[2]);
}

TEST(RingBuffer, CapacityOneAlwaysHoldsLatestElement) {
    RingBuffer<int> buf(1);
    for (int i = 1; i <= 5; ++i) {
        buf.push(i);
        EXPECT_EQ(1u, buf.size());
        EXPECT_TRUE(buf.is_full());

        std::vector<int> result;
        buf.iterate([&](const int& v) { result.push_back(v); });
        ASSERT_EQ(1u, result.size());
        EXPECT_EQ(i, result[0]);
    }
}

TEST(RingBuffer, OverwriteDoesNotChangeCapacity) {
    RingBuffer<int> buf(4);
    for (int i = 0; i < 20; ++i) {
        buf.push(i);
    }
    EXPECT_EQ(4u, buf.capacity());
    EXPECT_EQ(4u, buf.size());
    EXPECT_TRUE(buf.is_full());
}

// ---------------------------------------------------------------------------
// Wrap-around edge cases
// ---------------------------------------------------------------------------

TEST(RingBuffer, WrapAroundTailIterationIsCorrect) {
    // Fill exactly to capacity, then push one more to force a wrap.
    RingBuffer<int> buf(5);
    for (int i = 0; i < 5; ++i) {
        buf.push(i); // 0,1,2,3,4
    }
    buf.push(5); // wraps: 1,2,3,4,5

    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(5u, result.size());
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(i + 1, result[i]);
    }
}

TEST(RingBuffer, DoubleWrapAroundIterationIsCorrect) {
    RingBuffer<int> buf(4);
    // Push 2*capacity + 1 elements so the ring wraps twice.
    for (int i = 0; i < 9; ++i) {
        buf.push(i); // last 4 survive: 5,6,7,8
    }

    std::vector<int> result;
    buf.iterate([&](const int& v) { result.push_back(v); });

    ASSERT_EQ(4u, result.size());
    EXPECT_EQ(5, result[0]);
    EXPECT_EQ(6, result[1]);
    EXPECT_EQ(7, result[2]);
    EXPECT_EQ(8, result[3]);
}

// ---------------------------------------------------------------------------
// Template instantiation with non-trivial types
// ---------------------------------------------------------------------------

TEST(RingBuffer, WorksWithStdString) {
    RingBuffer<std::string> buf(3);
    buf.push("hello");
    buf.push("world");

    std::vector<std::string> result;
    buf.iterate([&](const std::string& s) { result.push_back(s); });

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("hello", result[0]);
    EXPECT_EQ("world", result[1]);
}

TEST(RingBuffer, OverwriteWithStdStringPreservesOrder) {
    RingBuffer<std::string> buf(2);
    buf.push("a");
    buf.push("b");
    buf.push("c"); // overwrites "a"

    std::vector<std::string> result;
    buf.iterate([&](const std::string& s) { result.push_back(s); });

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("b", result[0]);
    EXPECT_EQ("c", result[1]);
}

TEST(RingBuffer, WorksWithVector) {
    RingBuffer<std::vector<int>> buf(2);
    buf.push({1, 2, 3});
    buf.push({4, 5, 6});

    std::vector<std::vector<int>> result;
    buf.iterate([&](const std::vector<int>& v) { result.push_back(v); });

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ((std::vector<int>{1, 2, 3}), result[0]);
    EXPECT_EQ((std::vector<int>{4, 5, 6}), result[1]);
}

// ---------------------------------------------------------------------------
// iterate callback invocation count
// ---------------------------------------------------------------------------

TEST(RingBuffer, IterateCallbackInvokedExactlySizeTimes) {
    RingBuffer<int> buf(10);
    for (int i = 0; i < 7; ++i) {
        buf.push(i);
    }

    int count = 0;
    buf.iterate([&](const int&) { ++count; });
    EXPECT_EQ(7, count);
}

TEST(RingBuffer, IterateAfterOverflowCallbackInvokedCapacityTimes) {
    RingBuffer<int> buf(5);
    for (int i = 0; i < 12; ++i) {
        buf.push(i);
    }

    int count = 0;
    buf.iterate([&](const int&) { ++count; });
    EXPECT_EQ(5, count);
}
