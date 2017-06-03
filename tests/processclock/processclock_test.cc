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

#include <thread>

#include <gtest/gtest.h>
#include <platform/processclock.h>

#if defined(_MSC_VER) && _MSC_VER < 1900 /* less than Visual Studio 2015 */

struct MockProcessClock : ProcessClock {
    static ProcessClock::duration public_calculateDuration(
                               const LONGLONG freq, const LARGE_INTEGER count) {
        return ProcessClock::calculateDuration(freq, count);
    }
};

/**
 * Group of tests for validating that the ProcessClock::calculateDuration
 * works as expected.
 */

TEST(ProcessClockTest, DurationTest_Freq_1_Count_1) {
    LONGLONG freq{ 1 };
    LARGE_INTEGER count{ 1 };
    const auto duration = MockProcessClock::public_calculateDuration(freq,
                                                                     count);
    EXPECT_EQ(std::chrono::seconds(1), duration);
}

TEST(ProcessClockTest, DurationTest_Freq_1_Count_10) {
    LONGLONG freq{ 1 };
    LARGE_INTEGER count{ 10 };
    const auto duration = MockProcessClock::public_calculateDuration(freq,
                                                                     count);
    EXPECT_EQ(std::chrono::seconds(10), duration);
}

TEST(ProcessClockTest, DurationTest_Freq_10_Count_100) {
    LONGLONG freq{ 10 };
    LARGE_INTEGER count{ 100 };
    const auto duration = MockProcessClock::public_calculateDuration(freq,
                                                                     count);
    EXPECT_EQ(std::chrono::seconds(10), duration);
}

TEST(ProcessClockTest, DurationTest_Freq_100_Count_10) {
    LONGLONG freq{ 100 };
    LARGE_INTEGER count{ 10 };
    const auto duration = MockProcessClock::public_calculateDuration(freq,
                                                                     count);
    EXPECT_EQ(std::chrono::milliseconds(100), duration);
}

TEST(ProcessClockTest, DurationTest_Overflow) {
    LONGLONG freq{ LONGLONG(1) << 63 };
    LARGE_INTEGER count;
    count.QuadPart = LONGLONG(1) << 63;
    const auto duration = MockProcessClock::public_calculateDuration(freq,
                                                                     count);
    EXPECT_EQ(std::chrono::seconds(1), duration);
}

#endif

TEST(DefaultProcessClockSourceTest, SensibleBounds) {
    auto a = cb::ProcessClock::now();
    auto b = cb::defaultProcessClockSource().now();
    auto c = cb::ProcessClock::now();

    EXPECT_LE(a, b);
    EXPECT_LE(b, c);
}
