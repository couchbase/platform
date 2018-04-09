/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <gtest/gtest.h>
#include <platform/ring_buffer.h>
#include <vector>

using cb::RingBuffer;
using cb::RingBufferVector;

// Wrapper class that gives us a 'fixed' 10-element ring buffer.
template <typename T>
class TestRingBufferVector : public RingBufferVector<T> {
public:
    TestRingBufferVector<T>() : RingBufferVector<T>(10) {
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
TYPED_TEST_CASE(RingBufferTest, MyTypes);

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
