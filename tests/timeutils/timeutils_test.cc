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

#include <folly/portability/GTest.h>

using cb::text2time;
using cb::time2text;

TEST(Time2TextTest, Nano0) {
    EXPECT_EQ(std::string("0 ns"),
              time2text(std::chrono::nanoseconds(0)));
}

TEST(Time2TextTest, Nano9999) {
    EXPECT_EQ(std::string("9999 ns"),
              time2text(std::chrono::nanoseconds(9999)));
}

TEST(Time2TextTest, NanoUsecWrap) {
    EXPECT_EQ(std::string("10 us"),
              time2text(std::chrono::microseconds(10)));
}

TEST(Time2TextTest, Usec9999) {
    EXPECT_EQ(std::string("9999 us"),
              time2text(std::chrono::microseconds(9999)));
}

TEST(Time2TextTest, UsecMsecWrap) {
    EXPECT_EQ(std::string("10 ms"),
              time2text(std::chrono::milliseconds(10)));
}

TEST(Time2TextTest, Msec9999) {
    EXPECT_EQ(std::string("9999 ms"),
              time2text(std::chrono::milliseconds(9999)));
}

TEST(Time2TextTest, MsecSecWrap) {
    EXPECT_EQ(std::string("10 s"),
              time2text(std::chrono::seconds(10)));
}

TEST(Time2TextTest, SecLargest) {
    EXPECT_EQ(std::string("599 s"),
              time2text(std::chrono::seconds(599)));
}

TEST(Time2TextTest, AlmostFullSpecTime) {
    EXPECT_EQ(std::string("10m:0s"),
              time2text(std::chrono::minutes(10)));
}

TEST(Time2TextTest, FullSpecTime) {
    std::chrono::nanoseconds ns (std::chrono::hours(1)
                                 + std::chrono::minutes(1)
                                 + std::chrono::seconds(1));

    EXPECT_EQ(std::string("1h:1m:1s"), time2text(ns));
}

TEST(Text2timeTest, nanoseconds) {
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("1 ns"));
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("1ns"));
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("1 nanoseconds"));
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("1nanoseconds"));
    EXPECT_EQ(std::chrono::nanoseconds(12340), text2time("12340 ns"));
    EXPECT_EQ(std::chrono::nanoseconds(12340), text2time("12340 nanoseconds"));
}
TEST(Text2timeTest, microseconds) {
    EXPECT_EQ(std::chrono::microseconds(1), text2time("1 us"));
    EXPECT_EQ(std::chrono::microseconds(1), text2time("1us"));
    EXPECT_EQ(std::chrono::microseconds(1), text2time("1 microseconds"));
    EXPECT_EQ(std::chrono::microseconds(1), text2time("1microseconds"));
    EXPECT_EQ(std::chrono::microseconds(12340), text2time("12340 us"));
    EXPECT_EQ(std::chrono::microseconds(12340),
              text2time("12340 microseconds"));
}
TEST(Text2timeTest, milliseconds) {
    EXPECT_EQ(std::chrono::milliseconds(1), text2time("1 ms"));
    EXPECT_EQ(std::chrono::milliseconds(1), text2time("1ms"));
    EXPECT_EQ(std::chrono::milliseconds(1), text2time("1 milliseconds"));
    EXPECT_EQ(std::chrono::milliseconds(1), text2time("1milliseconds"));
    EXPECT_EQ(std::chrono::milliseconds(12340), text2time("12340 ms"));
    EXPECT_EQ(std::chrono::milliseconds(12340),
              text2time("12340 milliseconds"));
    EXPECT_EQ(std::chrono::milliseconds(654), text2time("   654  "));
}
TEST(Text2timeTest, seconds) {
    EXPECT_EQ(std::chrono::seconds(1), text2time("1 s"));
    EXPECT_EQ(std::chrono::seconds(1), text2time("1s"));
    EXPECT_EQ(std::chrono::seconds(1), text2time("1 seconds"));
    EXPECT_EQ(std::chrono::seconds(1), text2time("1seconds"));
    EXPECT_EQ(std::chrono::seconds(12340), text2time("12340 s"));
    EXPECT_EQ(std::chrono::seconds(12340), text2time("12340 seconds"));
}
TEST(Text2timeTest, minutes) {
    EXPECT_EQ(std::chrono::minutes(1), text2time("1 m"));
    EXPECT_EQ(std::chrono::minutes(1), text2time("1m"));
    EXPECT_EQ(std::chrono::minutes(1), text2time("1 minutes"));
    EXPECT_EQ(std::chrono::minutes(1), text2time("1minutes"));
    EXPECT_EQ(std::chrono::minutes(12340), text2time("12340 m"));
    EXPECT_EQ(std::chrono::minutes(12340), text2time("12340 minutes"));
}
TEST(Text2timeTest, hours) {
    EXPECT_EQ(std::chrono::hours(1), text2time("1 h"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1h"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1 hours"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1hours"));
    EXPECT_EQ(std::chrono::hours(12340), text2time("12340 h"));
    EXPECT_EQ(std::chrono::hours(12340), text2time("12340 hours"));
}

TEST(Text2timeTest, InvalidInput) {
    EXPECT_THROW(text2time(""), std::invalid_argument);
    EXPECT_THROW(text2time("a"), std::invalid_argument);
    EXPECT_THROW(text2time("!"), std::invalid_argument);
    EXPECT_THROW(text2time("2 units"), std::invalid_argument);
}
