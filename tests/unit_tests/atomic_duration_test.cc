/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/atomic_duration.h>

#include <folly/portability/GTest.h>

TEST(AtomicDurationTest, Constructors) {
    // Test the constructors. Implicitly tests load() and store() functions.
    cb::AtomicDuration<> atomicDurationDefault;
    cb::AtomicDuration<> atomicDurationValue(
            std::chrono::steady_clock::duration(20));
    cb::AtomicDuration<> atomicDurationCopy(atomicDurationValue);

    ASSERT_EQ(std::chrono::steady_clock::duration::zero(),
              atomicDurationDefault.load());
    ASSERT_EQ(std::chrono::steady_clock::duration(20),
              atomicDurationValue.load());
    ASSERT_EQ(atomicDurationValue.load(), atomicDurationCopy.load());
}

TEST(AtomicDurationTest, FetchAdd) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(10),
              atomicDuration.fetch_add(std::chrono::steady_clock::duration(
                      5))); // returns previous value
    EXPECT_EQ(std::chrono::steady_clock::duration(15), atomicDuration.load());
}

TEST(AtomicDurationTest, FetchSub) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(10),
              atomicDuration.fetch_sub(std::chrono::steady_clock::duration(
                      5))); // returns previous value
    EXPECT_EQ(std::chrono::steady_clock::duration(5), atomicDuration.load());
}

TEST(AtomicDurationTest, TypeCastOperator) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(10),
              static_cast<std::chrono::steady_clock::duration>(atomicDuration));
}

TEST(AtomicDurationTest, AssignOperator) {
    cb::AtomicDuration<> atomicDuration;
    atomicDuration = std::chrono::steady_clock::duration(10);
    EXPECT_EQ(std::chrono::steady_clock::duration(10), atomicDuration.load());
}

TEST(AtomicDurationTest, AddAssignOperator) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    atomicDuration += std::chrono::steady_clock::duration(5);
    EXPECT_EQ(std::chrono::steady_clock::duration(15), atomicDuration.load());
}

TEST(AtomicDurationTest, SubtractAssignOperator) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    atomicDuration -= std::chrono::steady_clock::duration(5);
    EXPECT_EQ(std::chrono::steady_clock::duration(5), atomicDuration.load());
}

TEST(AtomicDurationTest, PreIncrement) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(11), ++atomicDuration);
}

TEST(AtomicDurationTest, PostIncrement) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(10),
              atomicDuration++); // returns previous value
    EXPECT_EQ(std::chrono::steady_clock::duration(11), atomicDuration.load());
}

TEST(AtomicDurationTest, PreDecrement) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(9), --atomicDuration);
}

TEST(AtomicDurationTest, PostDecrement) {
    cb::AtomicDuration<> atomicDuration(
            std::chrono::steady_clock::duration(10));
    EXPECT_EQ(std::chrono::steady_clock::duration(10),
              atomicDuration--); // returns previous value
    EXPECT_EQ(std::chrono::steady_clock::duration(9), atomicDuration.load());
}
