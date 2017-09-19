/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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

#include "config.h"

#include <platform/non_negative_counter.h>

#include <gtest/gtest.h>

TEST(NonNegativeCounterTest, Increment) {
    cb::NonNegativeCounter<size_t> nnAtomic(1);
    ASSERT_EQ(1, nnAtomic);

    EXPECT_EQ(2, ++nnAtomic);
    EXPECT_EQ(2, nnAtomic++);
    EXPECT_EQ(3, nnAtomic);
}

TEST(NonNegativeCounterTest, Add) {
    cb::NonNegativeCounter<size_t> nnAtomic(1);
    ASSERT_EQ(1, nnAtomic);

    EXPECT_EQ(3, nnAtomic += 2);
    EXPECT_EQ(3, nnAtomic.fetch_add(2));
    EXPECT_EQ(5, nnAtomic);
}

TEST(NonNegativeCounterTest, Decrement) {
    cb::NonNegativeCounter<size_t> nnAtomic(2);
    ASSERT_EQ(2, nnAtomic);

    EXPECT_EQ(1, --nnAtomic);
    EXPECT_EQ(1, nnAtomic--);
    EXPECT_EQ(0, nnAtomic);
}

TEST(NonNegativeCounterTest, Subtract) {
    cb::NonNegativeCounter<size_t> nnAtomic(4);
    ASSERT_EQ(4, nnAtomic);

    EXPECT_EQ(2, nnAtomic -= 2);
    EXPECT_EQ(2, nnAtomic.fetch_sub(2));
    EXPECT_EQ(0, nnAtomic);
}

// Test that a NonNegativeCounter will clamp to zero.
TEST(NonNegativeCounterTest, ClampsToZero) {
    cb::NonNegativeCounter<size_t, cb::ClampAtZeroUnderflowPolicy> nnAtomic(0);

    EXPECT_EQ(0, --nnAtomic);
    EXPECT_EQ(0, nnAtomic--);
    EXPECT_EQ(0, nnAtomic);

    nnAtomic = 5;
    EXPECT_EQ(5, nnAtomic.fetch_sub(10)); // returns previous value
    EXPECT_EQ(0, nnAtomic); // has been clamped to zero
}

// Test the ThrowException policy.
TEST(NonNegativeCounterTest, ThrowExceptionPolicy) {
    cb::NonNegativeCounter<size_t, cb::ThrowExceptionUnderflowPolicy> nnAtomic(0);

    EXPECT_THROW(--nnAtomic, std::underflow_error);
    EXPECT_EQ(0, nnAtomic);
    EXPECT_THROW(nnAtomic--, std::underflow_error);
    EXPECT_EQ(0, nnAtomic);

    nnAtomic = 1;
    EXPECT_THROW(nnAtomic -= 2, std::underflow_error);
    EXPECT_EQ(1, nnAtomic);
}
