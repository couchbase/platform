/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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

#include <relaxed_atomic.h>

#include <gtest/gtest.h>

// Test that we can use RelaxedAtomic<T> in STL containers.
TEST(RelaxedAtomicTest, STLContainer) {
    std::vector<Couchbase::RelaxedAtomic<uint64_t>> vec;

    // Check we can populate a std::vector with assign()
    vec.assign(3, 1);
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(1, vec[1]);
    EXPECT_EQ(1, vec[2]);

    // Check we can change existing values
    vec[2] = 2;
    EXPECT_EQ(2, vec[2]);
}

TEST(RelaxedAtomicTest, setIfSmaller) {
    Couchbase::RelaxedAtomic<uint8_t> val;
    val.store(10);

    // Check we don't store larger numbers
    val.setIfSmaller(15);
    EXPECT_EQ(val.load(), 10);

    // Check we store smaller numbers
    val.setIfSmaller(5);
    EXPECT_EQ(val.load(), 5);

    Couchbase::RelaxedAtomic<uint8_t> smaller;
    smaller.store(3);

    // Check we can correctly store from another Couchbase::RelaxedAtomic
    val.setIfSmaller(smaller);
    EXPECT_EQ(val.load(), 3);
}
