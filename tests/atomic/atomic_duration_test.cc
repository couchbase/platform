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

#include <platform/atomic_duration.h>

#include <gtest/gtest.h>

TEST(AtomicDurationTest, Constructors) {
    // Test the constructors. Implicitly tests load() and store() functions.
    cb::AtomicDuration atomicDurationDefault;
    cb::AtomicDuration atomicDurationValue(ProcessClock::duration(20));
    cb::AtomicDuration atomicDurationCopy(atomicDurationValue);

    ASSERT_EQ(ProcessClock::duration::zero(), atomicDurationDefault.load());
    ASSERT_EQ(ProcessClock::duration(20), atomicDurationValue.load());
    ASSERT_EQ(atomicDurationValue.load(), atomicDurationCopy.load());
}

TEST(AtomicDurationTest, FetchAdd) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(10),
              atomicDuration.fetch_add(
                      ProcessClock::duration(5))); // returns previous value
    EXPECT_EQ(ProcessClock::duration(15), atomicDuration.load());
}

TEST(AtomicDurationTest, FetchSub) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(10),
              atomicDuration.fetch_sub(
                      ProcessClock::duration(5))); // returns previous value
    EXPECT_EQ(ProcessClock::duration(5), atomicDuration.load());
}

TEST(AtomicDurationTest, TypeCastOperator) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(10),
              static_cast<ProcessClock::duration>(atomicDuration));
}

TEST(AtomicDurationTest, AssignOperator) {
    cb::AtomicDuration atomicDuration;
    atomicDuration = ProcessClock::duration(10);
    EXPECT_EQ(ProcessClock::duration(10), atomicDuration.load());
}

TEST(AtomicDurationTest, AddAssignOperator) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    atomicDuration += ProcessClock::duration(5);
    EXPECT_EQ(ProcessClock::duration(15), atomicDuration.load());
}

TEST(AtomicDurationTest, SubtractAssignOperator) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    atomicDuration -= ProcessClock::duration(5);
    EXPECT_EQ(ProcessClock::duration(5), atomicDuration.load());
}

TEST(AtomicDurationTest, PreIncrement) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(11), ++atomicDuration);
}

TEST(AtomicDurationTest, PostIncrement) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(10),
              atomicDuration++); // returns previous value
    EXPECT_EQ(ProcessClock::duration(11), atomicDuration.load());
}

TEST(AtomicDurationTest, PreDecrement) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(9), --atomicDuration);
}

TEST(AtomicDurationTest, PostDecrement) {
    cb::AtomicDuration atomicDuration(ProcessClock::duration(10));
    EXPECT_EQ(ProcessClock::duration(10),
              atomicDuration--); // returns previous value
    EXPECT_EQ(ProcessClock::duration(9), atomicDuration.load());
}
