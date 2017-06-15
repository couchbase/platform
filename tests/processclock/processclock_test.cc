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

TEST(DefaultProcessClockSourceTest, SensibleBounds) {
    auto a = cb::ProcessClock::now();
    auto b = cb::defaultProcessClockSource().now();
    auto c = cb::ProcessClock::now();

    EXPECT_LE(a, b);
    EXPECT_LE(b, c);
}

TEST(to_string, ns) {
    EXPECT_EQ("0ns", to_string(std::chrono::nanoseconds(0)));
    EXPECT_EQ("9999ns", to_string(std::chrono::nanoseconds(9999)));
}

TEST(to_string, usec) {
    EXPECT_EQ("10µs", to_string(std::chrono::nanoseconds(10000)));
    EXPECT_EQ("9999µs", to_string(std::chrono::microseconds(9999)));
}


TEST(to_string, ms) {
    EXPECT_EQ("10ms", to_string(std::chrono::microseconds(10000)));
    EXPECT_EQ("9999ms", to_string(std::chrono::milliseconds(9999)));
}

TEST(to_string, ss) {
    EXPECT_EQ("10s", to_string(std::chrono::milliseconds(10000)));
    EXPECT_EQ("599s", to_string(std::chrono::seconds(599)));
    // values greater than 600 should be printed as h:m:s
    EXPECT_EQ("0:10:0", to_string(std::chrono::seconds(600)));
}
