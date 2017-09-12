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
#include <platform/timeutils.h>
#include <chrono>

#include <gtest/gtest.h>

using cb::time2text;

TEST(TimeutilsTest, Nano0) {
    EXPECT_EQ(std::string("0 ns"),
              time2text(std::chrono::nanoseconds(0)));
}

TEST(TimeutilsTest, Nano9999) {
    EXPECT_EQ(std::string("9999 ns"),
              time2text(std::chrono::nanoseconds(9999)));
}

TEST(TimeutilsTest, NanoUsecWrap) {
    EXPECT_EQ(std::string("10 us"),
              time2text(std::chrono::microseconds(10)));
}

TEST(TimeutilsTest, Usec9999) {
    EXPECT_EQ(std::string("9999 us"),
              time2text(std::chrono::microseconds(9999)));
}

TEST(TimeutilsTest, UsecMsecWrap) {
    EXPECT_EQ(std::string("10 ms"),
              time2text(std::chrono::milliseconds(10)));
}

TEST(TimeutilsTest, Msec9999) {
    EXPECT_EQ(std::string("9999 ms"),
              time2text(std::chrono::milliseconds(9999)));
}

TEST(TimeutilsTest, MsecSecWrap) {
    EXPECT_EQ(std::string("10 s"),
              time2text(std::chrono::seconds(10)));
}

TEST(TimeutilsTest, SecLargest) {
    EXPECT_EQ(std::string("599 s"),
              time2text(std::chrono::seconds(599)));
}

TEST(TimeutilsTest, AlmostFullSpecTime) {
    EXPECT_EQ(std::string("10m:0s"),
              time2text(std::chrono::minutes(10)));
}

TEST(TimeutilsTest, FullSpecTime) {
    std::chrono::nanoseconds ns (std::chrono::hours(1)
                                 + std::chrono::minutes(1)
                                 + std::chrono::seconds(1));

    EXPECT_EQ(std::string("1h:1m:1s"), time2text(ns));
}
