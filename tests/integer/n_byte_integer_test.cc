/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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
#include <platform/n_byte_integer.h>

#include <folly/portability/GTest.h>

TEST(NByteCounterTest, basic) {
    cb::UnsignedNByteInteger<6> counter;
    EXPECT_EQ(6, sizeof(counter));

    EXPECT_EQ(0, counter);
    counter += 1;
    EXPECT_EQ(1, counter);
    EXPECT_EQ(1, counter++);
    EXPECT_EQ(2, counter);
    EXPECT_EQ(3, ++counter);
    EXPECT_EQ(3, counter--);
    EXPECT_EQ(2, counter);
    EXPECT_EQ(1, --counter);
    EXPECT_EQ(1, counter);
}

// Designed behaviour is to truncate when initialising from 8-bytes
TEST(NByteCounterTest, truncate) {
    cb::UnsignedNByteInteger<6> counter(0x0080A80000000000);
    EXPECT_EQ(0x0000A80000000000, counter);
}

TEST(NByteCounterTest, overflow) {
    cb::UnsignedNByteInteger<6> counter(0x0080ffffffffffff);
    EXPECT_EQ(0x0000ffffffffffff, counter);
    counter += 2;
    EXPECT_EQ(1, counter);
}

TEST(NByteCounterTest, underflow) {
    cb::UnsignedNByteInteger<6> counter(0x0080ffffffffffff);
    EXPECT_EQ(0x0000ffffffffffff, counter);
    counter -= 0x0080ffffffffffff;
    EXPECT_EQ(0, counter);
    counter -= 1;
    EXPECT_EQ(0x0000ffffffffffff, counter);
}
