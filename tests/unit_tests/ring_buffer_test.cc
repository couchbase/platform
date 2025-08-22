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
#include <platform/ring_buffer.h>
#include <vector>

using cb::RingBuffer;
using cb::RingBufferVector;

// Wrapper class that gives us a 'fixed' 10-element ring buffer.
template <typename T>
class TestRingBufferVector : public RingBufferVector<T> {
public:
    TestRingBufferVector() : RingBufferVector<T>(10) {
    }
    void reset() {
        RingBufferVector<T>::reset(10);
    }
};

template <typename T>
class RingBufferTest : public ::testing::Test {
public:
    typedef T RingBufferType;
};

typedef ::testing::Types<RingBuffer<int, 10>, TestRingBufferVector<int>>
    MyTypes;
TYPED_TEST_SUITE(RingBufferTest, MyTypes);

TYPED_TEST(RingBufferTest, testRingBuffer) {
    TypeParam rb;
    ASSERT_EQ(10u, rb.size());
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(0, rb[i]);
    }

    rb.push_back(42);
    ASSERT_EQ(42, rb.back());
    ASSERT_EQ(0, rb.front());
    ASSERT_EQ(10u, rb.size());

    // Fill up the ring buffer
    for (int i = 0; i < 10; i++) {
        rb.push_back((i + 1) * 100);
    }

    // Test iteration
    std::vector<int> nums;
    for (auto num : rb) {
        nums.push_back(num);
    }
    ASSERT_EQ(10u, nums.size());

    // Test iteration on const values
    std::vector<int> const_nums;
    for (const auto& num : rb) {
        const_nums.push_back(num);
    }

    for (int i = 0; i < 10; i++) {
        int exp = (i + 1) * 100;
        ASSERT_EQ(exp, nums[i]);
        ASSERT_EQ(exp, const_nums[i]);
    }

    rb.reset();
    for (auto num : rb) {
        ASSERT_EQ(0, num);
    }

    for (int i = 0; i < 10; i++) {
        rb.emplace_back(i + 1);
    }

    ASSERT_EQ(10u, rb.size());
    for (size_t i = 0; i < rb.size(); ++i) {
        ASSERT_EQ(int(i) + 1, rb[i]);
    }
}
