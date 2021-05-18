/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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

TEST(NByteCounterTest, byteSwap) {
    cb::UnsignedNByteInteger<6> counter(0x123456789ABC);
    auto swapped = counter.byteSwap();
    EXPECT_EQ(0xBC9A78563412, swapped);
}

TEST(NByteCounterTest, ntoh) {
    cb::UnsignedNByteInteger<6> counter(12345);
    EXPECT_EQ(0x3039, counter);

    auto networkOrder = counter.hton();
    if (folly::kIsLittleEndian) {
        EXPECT_EQ(0x393000000000, networkOrder);
    } else {
        EXPECT_EQ(0x3039, counter);
    }

    auto hostOrder = networkOrder.ntoh();
    EXPECT_EQ(0x3039, hostOrder);
}