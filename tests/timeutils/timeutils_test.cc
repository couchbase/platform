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

#include <gtest/gtest.h>

using Couchbase::hrtime2text;

TEST(TimeutilsTest, Nano0) {
    hrtime_t ns = 0;
    EXPECT_EQ(std::string("0 ns"), hrtime2text(ns));
}

TEST(TimeutilsTest, Nano9999) {
    hrtime_t ns = 9999;
    EXPECT_EQ(std::string("9999 ns"), hrtime2text(ns));
}

TEST(TimeutilsTest, NanoUsecWrap) {
    hrtime_t ns = 9999 + 1;
    EXPECT_EQ(std::string("10 us"), hrtime2text(ns));
}

TEST(TimeutilsTest, Usec9999) {
    hrtime_t ns = 9999;
    ns *= 1000; // make usec
    EXPECT_EQ(std::string("9999 us"), hrtime2text(ns));
}

TEST(TimeutilsTest, UsecMsecWrap) {
    hrtime_t ns = 9999 + 1;
    ns *= 1000; // make usec
    EXPECT_EQ(std::string("10 ms"), hrtime2text(ns));
}

TEST(TimeutilsTest, Msec9999) {
    hrtime_t ns = 9999;
    ns *= 1000; // make usec
    ns *= 1000; // make ms

    EXPECT_EQ(std::string("9999 ms"), hrtime2text(ns));
}

TEST(TimeutilsTest, MsecSecWrap) {
    hrtime_t ns = 9999 + 1;
    ns *= 1000; // make usec
    ns *= 1000; // make ms

    EXPECT_EQ(std::string("10 s"), hrtime2text(ns));
}

TEST(TimeutilsTest, SecLargest) {
    hrtime_t ns = 1000;
    ns *= 1000; // make usec
    ns *= 1000; // make ms
    ns *= 599;

    EXPECT_EQ(std::string("599 s"), hrtime2text(ns));
}

TEST(TimeutilsTest, AlmostFullSpecTime) {
    hrtime_t ns = 1000;
    ns *= 1000; // make usec
    ns *= 1000; // make ms
    ns *= 600;  // 10 minutes

    EXPECT_EQ(std::string("10m:0s"), hrtime2text(ns));
}

TEST(TimeutilsTest, FullSpecTime) {
    hrtime_t ns = 1000;
    ns *= 1000; // make usec
    ns *= 1000; // make ms
    ns *= 3661; // 1hour, 1minute and 1sec

    EXPECT_EQ(std::string("1h:1m:1s"), hrtime2text(ns));
}
