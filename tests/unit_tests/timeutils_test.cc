/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/timeutils.h>
#include <chrono>
#include <fmt/chrono.h>
#include <folly/Chrono.h>
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

TEST(Time2TextTest, NanoUsecRound) {
    EXPECT_EQ(std::string("10 us"),
              time2text(std::chrono::nanoseconds(10499)));
    EXPECT_EQ(std::string("11 us"),
              time2text(std::chrono::nanoseconds(10501)));
    EXPECT_EQ(std::string("11 us"),
              time2text(std::chrono::nanoseconds(10999)));
}

TEST(Time2TextTest, Usec9999) {
    EXPECT_EQ(std::string("9999 us"),
              time2text(std::chrono::microseconds(9999)));
}

TEST(Time2TextTest, UsecMsecWrap) {
    EXPECT_EQ(std::string("10 ms"),
              time2text(std::chrono::milliseconds(10)));
}

TEST(Time2TextTest, UsecMsecRound) {
    EXPECT_EQ(std::string("10 ms"),
              time2text(std::chrono::microseconds(10499)));
    EXPECT_EQ(std::string("11 ms"),
              time2text(std::chrono::microseconds(10501)));
    EXPECT_EQ(std::string("11 ms"),
              time2text(std::chrono::microseconds(10999)));
}

TEST(Time2TextTest, Msec9999) {
    EXPECT_EQ(std::string("9999 ms"),
              time2text(std::chrono::milliseconds(9999)));
}

TEST(Time2TextTest, MsecSecWrap) {
    EXPECT_EQ(std::string("10 s"),
              time2text(std::chrono::seconds(10)));
}

TEST(Time2TextTest, MsecSecRound) {
    EXPECT_EQ(std::string("10 s"),
              time2text(std::chrono::milliseconds(10499)));
    EXPECT_EQ(std::string("11 s"),
              time2text(std::chrono::milliseconds(10501)));
    EXPECT_EQ(std::string("11 s"),
              time2text(std::chrono::milliseconds(10999)));
}

TEST(Time2TextTest, SecLargest) {
    EXPECT_EQ(std::string("599 s"),
              time2text(std::chrono::seconds(599)));
}

TEST(Time2TextTest, NsecSecRound) {
    EXPECT_EQ(std::string("10 s"),
              time2text(std::chrono::nanoseconds(10499999999)));
    EXPECT_EQ(std::string("11 s"),
              time2text(std::chrono::nanoseconds(10500000001)));
    EXPECT_EQ(std::string("11 s"),
              time2text(std::chrono::nanoseconds(10999999999)));
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

TEST(Text2timeTest, Mixed_1) {
    const auto mytime = std::chrono::hours(2) + std::chrono::minutes(15) +
                        std::chrono::seconds(4);
    EXPECT_EQ("2h:15m:4s", time2text(mytime));
    EXPECT_EQ(mytime, text2time("2h:15m:4s"));
}

TEST(Text2timeTest, Mixed_2) {
    const auto mytime = std::chrono::minutes(15) + std::chrono::seconds(4);
    EXPECT_EQ("15m:4s", time2text(mytime));
    EXPECT_EQ(mytime, text2time("15m:4s"));
}

TEST(Text2timeTest, Mixed_3) {
    const auto mytime =
            std::chrono::seconds(4) + std::chrono::milliseconds{320};
    EXPECT_EQ("4320 ms", time2text(mytime));
    EXPECT_EQ(mytime, text2time("4s:320ms"));
}

TEST(Text2timeTest, Mixed_4) {
    const auto mytime =
            std::chrono::seconds(4) + std::chrono::milliseconds{320} +
            std::chrono::microseconds{50} + std::chrono::nanoseconds{101};
    EXPECT_EQ(mytime, text2time("4s:320ms:50us:101ns"));
}

TEST(Text2timeTest, Mixed_Whitespace) {
    const auto mytime = std::chrono::hours(2) + std::chrono::minutes{4} +
                        std::chrono::microseconds{50};
    EXPECT_EQ(mytime, text2time(" 2  h :  4 m  :50us"));
}

TEST(Text2timeTest, InvalidInput) {
    EXPECT_THROW(text2time(""), std::invalid_argument);
    EXPECT_THROW(text2time("a"), std::invalid_argument);
    EXPECT_THROW(text2time("!"), std::invalid_argument);
    EXPECT_THROW(text2time("2 units"), std::invalid_argument);
    EXPECT_THROW(text2time(" 2  h :4m:\n00000s"), std::invalid_argument);
}

/// Test clock which always advances by 10 nanoseconds every time now() is called.
struct TestClock {
    using rep = std::chrono::nanoseconds::rep;
    using period = std::chrono::nanoseconds::period;
    using duration = std::chrono::nanoseconds;
    using time_point = std::chrono::time_point<TestClock, duration>;
    constexpr static bool is_steady = true;

    static time_point now() noexcept {
        static rep ticks = 0;
        return time_point{duration{ticks += 10}};
    }
};

// Verify the estimate calculation is correct, using a test clock as the
// measuring clock which always advances by 10ns.
TEST(EstimateClockOverhead, Calculation) {
    // Request 5 samples; given TestClock::now advances by 10ns each
    // call we expect to get an estimate of 10 / 5 = 2ns.
    auto result =
            cb::estimateClockOverhead<std::chrono::steady_clock, TestClock>(5);

    EXPECT_EQ(2, result.overhead.count())
            << "Expected estimate of 2 when TestClock "
               "used which always ticks by a fixed amount.";
}

TEST(EstimateClockOverhead, SteadyClock) {
    auto result = cb::estimateClockOverhead<std::chrono::steady_clock>();
    // Difficult to test with a real clock which will vary based on environment
    // system load etc, just perform some basic sanity checks.

    // Expect to have a non-zero number of nanoseconds to read the clock.
    EXPECT_NE(0, result.overhead.count());
    EXPECT_EQ(std::chrono::steady_clock::duration{1}, result.measurementPeriod);

    fmt::print("estimateClockOverhead(steady_clock) overhead: {}\n",
               result.overhead);
}

TEST(EstimateClockOverhead, CoarseSteadyClock) {
    auto result =
            cb::estimateClockOverhead<folly::chrono::coarse_steady_clock>();
    // Difficult to test with a real clock which will vary based on environment
    // system load etc, just perform some basic sanity checks.

    // Expect to have a non-zero number of nanoseconds to read the clock.
    EXPECT_NE(0, result.overhead.count());
    EXPECT_EQ(std::chrono::steady_clock::duration{1}, result.measurementPeriod);

    fmt::print("estimateClockOverhead(coarse_steady_clock) overhead: {}\n",
               result.overhead);
}
