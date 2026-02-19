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
    EXPECT_EQ(std::string("-10 us"),
              time2text(std::chrono::microseconds(-10)));
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
    EXPECT_EQ(std::string("-10 ms"),
              time2text(std::chrono::milliseconds(-10)));
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
    EXPECT_EQ(std::string("-10 s"),
              time2text(std::chrono::seconds(-10)));
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
    EXPECT_EQ(std::string("-10m:0s"),
              time2text(std::chrono::minutes(-10)));
}

TEST(Time2TextTest, FullSpecTime) {
    std::chrono::nanoseconds ns (std::chrono::hours(1)
                                 + std::chrono::minutes(1)
                                 + std::chrono::seconds(1));

    EXPECT_EQ(std::string("1h:1m:1s"), time2text(ns));
    EXPECT_EQ(std::string("-1h:1m:1s"), time2text(-ns));
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
    EXPECT_EQ(std::chrono::minutes(1440), text2time("1440 m"));
}
TEST(Text2timeTest, hours) {
    EXPECT_EQ(std::chrono::hours(1), text2time("1 h"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1h"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1 hours"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1hours"));
    EXPECT_EQ(std::chrono::hours(12340), text2time("12340 h"));
    EXPECT_EQ(std::chrono::hours(12340), text2time("12340 hours"));
    EXPECT_EQ(std::chrono::hours(24), text2time("24 h"));
    EXPECT_EQ(std::chrono::hours(8760), text2time("8760 h"));
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

// Happy Path Tests

TEST(Text2timeTest, HappyPath_AllUnitsWithLeadingZeros) {
    EXPECT_EQ(std::chrono::nanoseconds(0), text2time("0 ns"));
    EXPECT_EQ(std::chrono::microseconds(0), text2time("0 us"));
    EXPECT_EQ(std::chrono::milliseconds(0), text2time("0 ms"));
    EXPECT_EQ(std::chrono::seconds(0), text2time("0 s"));
    EXPECT_EQ(std::chrono::minutes(0), text2time("0 m"));
    EXPECT_EQ(std::chrono::hours(0), text2time("0 h"));

    EXPECT_EQ(std::chrono::nanoseconds(42), text2time("00042 ns"));
    EXPECT_EQ(std::chrono::microseconds(42), text2time("00042 us"));
    EXPECT_EQ(std::chrono::milliseconds(42), text2time("00042 ms"));
    EXPECT_EQ(std::chrono::seconds(42), text2time("00042 s"));
    EXPECT_EQ(std::chrono::minutes(42), text2time("00042 m"));
    EXPECT_EQ(std::chrono::hours(42), text2time("00042 h"));
}

TEST(Text2timeTest, HappyPath_DefaultMilliseconds) {
    EXPECT_EQ(std::chrono::milliseconds(0), text2time("0"));
    EXPECT_EQ(std::chrono::milliseconds(100), text2time("100"));
    EXPECT_EQ(std::chrono::milliseconds(12345), text2time("12345"));
}

TEST(Text2timeTest, HappyPath_ZeroInColonSeparated) {
    EXPECT_EQ(std::chrono::hours(1) + std::chrono::seconds(30),
              text2time("1h:0m:30s"));
    EXPECT_EQ(std::chrono::hours(1) + std::chrono::minutes(30),
              text2time("1h:30m:0s"));
    EXPECT_EQ(std::chrono::milliseconds(0), text2time("0ms:0ns"));
}

// Negative Value Tests

TEST(Text2timeTest, NegativeValues_AllUnits) {
    EXPECT_EQ(std::chrono::nanoseconds(-100), text2time("-100 ns"));
    EXPECT_EQ(std::chrono::microseconds(-100), text2time("-100 us"));
    EXPECT_EQ(std::chrono::milliseconds(-100), text2time("-100 ms"));
    EXPECT_EQ(std::chrono::seconds(-100), text2time("-100 s"));
    EXPECT_EQ(std::chrono::minutes(-100), text2time("-100 m"));
    EXPECT_EQ(std::chrono::hours(-100), text2time("-100 h"));
}

TEST(Text2timeTest, NegativeValues_DefaultMilliseconds) {
    EXPECT_EQ(std::chrono::milliseconds(-100), text2time("-100"));
}

TEST(Text2timeTest, NegativeValues_ColonSeparated) {
    // Individual components cannot have negative signs
    // Only the entire duration can be negative via a leading '-'
    EXPECT_THROW(text2time("-2h:-30m:-45s"), std::invalid_argument);
    EXPECT_THROW(text2time("2h:-30m"), std::invalid_argument);
    EXPECT_THROW(text2time("2h:30m:-45s"), std::invalid_argument);
}

TEST(Text2timeTest, NegativeValues_ColonSeparatedMixedSigns) {
    // Mixed signs on individual components are not supported
    // Only a single leading '-' is supported for the entire duration
    EXPECT_THROW(text2time("1h:-30m"), std::invalid_argument);
    EXPECT_THROW(text2time("-1h:-30m"), std::invalid_argument);
    // Note: "-1h:30m" is valid - it means the entire duration -(1h + 30m)
    EXPECT_EQ(-(std::chrono::hours(1) + std::chrono::minutes(30)),
              text2time("-1h:30m"));
}

// Failure Path Tests

TEST(Text2timeTest, FailurePath_WhitespaceOnly) {
    EXPECT_THROW(text2time("   "), std::invalid_argument);
    EXPECT_THROW(text2time("\t"), std::invalid_argument);
    EXPECT_THROW(text2time("\n"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_InvalidCharacters) {
    EXPECT_THROW(text2time("abc"), std::invalid_argument);
    EXPECT_THROW(text2time("xyz ns"), std::invalid_argument);
    EXPECT_THROW(text2time("hello world"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_InvalidNumericFormat) {
    EXPECT_THROW(text2time("1.5 s"), std::invalid_argument);
    EXPECT_THROW(text2time("--100 ns"), std::invalid_argument);
    EXPECT_THROW(text2time("++100 ns"), std::invalid_argument);
    EXPECT_THROW(text2time("1e5 s"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_IntegerOverflow) {
    // Test with numbers beyond int32 range
    EXPECT_THROW(text2time("999999999999999999 ns"), std::out_of_range);
    EXPECT_THROW(text2time("9999999999 s"), std::out_of_range);
}

TEST(Text2timeTest, FailurePath_InvalidUnitSpecifier) {
    EXPECT_THROW(text2time("100 xyz"), std::invalid_argument);
    EXPECT_THROW(text2time("100 sec"), std::invalid_argument);
    EXPECT_THROW(text2time("100 hr"), std::invalid_argument);
    EXPECT_THROW(text2time("100 min"), std::invalid_argument);
    EXPECT_THROW(text2time("100 nano"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_ColonSeparatedEmptyParts) {
    // Empty parts in colon-separated format are treated as zero nanoseconds
    // So ":" = "" + "" = 0 ns
    EXPECT_EQ(std::chrono::nanoseconds(0), text2time(":"));
    EXPECT_EQ(std::chrono::nanoseconds(0), text2time("::"));
    EXPECT_EQ(std::chrono::hours(1) + std::chrono::minutes(30),
              text2time("1h::30m"));
    EXPECT_EQ(std::chrono::minutes(30), text2time(":30m"));
    EXPECT_EQ(std::chrono::hours(1), text2time("1h:"));
}

TEST(Text2timeTest, FailurePath_ColonSeparatedInvalidParts) {
    EXPECT_THROW(text2time("abc:30m"), std::invalid_argument);
    EXPECT_THROW(text2time("1h:xyz:30s"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_NewlineInInput) {
    EXPECT_THROW(text2time("100\nns"), std::invalid_argument);
    EXPECT_THROW(text2time("1h:\n2m"), std::invalid_argument);
    EXPECT_THROW(text2time("1h:2m:\n3s"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_TabInInput) {
    EXPECT_THROW(text2time("\t"), std::invalid_argument);
    EXPECT_THROW(text2time("100\tms"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_SpecialCharacters) {
    EXPECT_THROW(text2time("100$ns"), std::invalid_argument);
    EXPECT_THROW(text2time("100&ms"), std::invalid_argument);
    EXPECT_THROW(text2time("100%us"), std::invalid_argument);
}

TEST(Text2timeTest, FailurePath_PartialUnitNames) {
    EXPECT_THROW(text2time("100 n"), std::invalid_argument);
    EXPECT_THROW(text2time("100 u"), std::invalid_argument);
    // Note: "100 m" and "100 h" are valid and return minutes/hours, tested
    // separately
}

TEST(Text2timeTest, FailurePath_DelimiterInSpecifier) {
    EXPECT_THROW(text2time("100 n:s"), std::invalid_argument);
    EXPECT_THROW(text2time("100 m:inutes"), std::invalid_argument);
}

// Edge Case Tests

TEST(Text2timeTest, EdgeCase_LargeInt32) {
    // Test near int32 max/min boundaries (within range)
    EXPECT_EQ(std::chrono::nanoseconds(2147483647), text2time("2147483647 ns"));
    // -2147483648 exceeds int range (abs(INT_MIN) > INT_MAX)
    EXPECT_THROW(text2time("-2147483648 ns"), std::out_of_range);
}

TEST(Text2timeTest, EdgeCase_MaxSafeInteger) {
    // Test with maximum values that fit in int but might cause overflow in
    // chrono
    EXPECT_EQ(std::chrono::seconds(2147483647), text2time("2147483647 s"));
    EXPECT_EQ(std::chrono::milliseconds(2147483647),
              text2time("2147483647 ms"));
    EXPECT_EQ(std::chrono::microseconds(2147483647),
              text2time("2147483647 us"));
    EXPECT_EQ(std::chrono::nanoseconds(2147483647), text2time("2147483647 ns"));
}

TEST(Text2timeTest, EdgeCase_VerySmallPositive) {
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("1 ns"));
    EXPECT_EQ(std::chrono::nanoseconds(1), text2time("0001 ns"));
}

TEST(Text2timeTest, EdgeCase_MultipleColonsWithDefaults) {
    // Using default milliseconds for number-only parts in colon-separated
    // format
    auto expected =
            std::chrono::milliseconds(1000) + std::chrono::milliseconds(500);
    // Note: This test assumes "1000:500" splits to ["1000", "500"] both treated
    // as milliseconds
    EXPECT_EQ(expected, text2time("1000:500"));

    // More tests for default milliseconds in colon-separated format
    EXPECT_EQ(std::chrono::milliseconds(15000), text2time("10000:5000"));
    EXPECT_EQ(std::chrono::milliseconds(0), text2time("0:0"));
    EXPECT_EQ(std::chrono::milliseconds(1), text2time("0:1"));

    // Mixed: explicit units with default milliseconds
    auto mixed_expected =
            std::chrono::seconds(1) + std::chrono::milliseconds(500);
    EXPECT_EQ(mixed_expected, text2time("1s:500"));

    auto mixed_expected2 =
            std::chrono::milliseconds(1000) + std::chrono::microseconds(500);
    EXPECT_EQ(mixed_expected2, text2time("1000:500us"));

    auto mixed_expected3 = std::chrono::milliseconds(1000) +
                           std::chrono::milliseconds(500) +
                           std::chrono::nanoseconds(100);
    EXPECT_EQ(mixed_expected3, text2time("1000:500:100ns"));
}

TEST(Text2timeTest, EdgeCase_TabAndSpacesMixedWhitespace) {
    // trim_space only trims space characters, not tabs
    EXPECT_THROW(text2time(" \t 42 ns \t "), std::invalid_argument);
    EXPECT_EQ(std::chrono::nanoseconds(42), text2time("  42 ns  "));
}

TEST(Text2timeTest, EdgeCase_ColonWithWhitespace) {
    auto expected = std::chrono::hours(1) + std::chrono::minutes(30);
    EXPECT_EQ(expected, text2time(" 1 h : 30 m "));
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

// Verify the resolution calculation is correct, using a test clock as the
// measuring clock which always advances by 10ns.
TEST(EstimateClockResolution, Calculation) {
    auto result = cb::estimateClockResolution<TestClock>();

    EXPECT_EQ(10, result.count())
            << "Expected estimated resolution of 10 TestClock used which "
               "always ticks by a fixed amount.";
}

TEST(EstimateClockResolution, SteadyClock) {
    auto result = cb::estimateClockResolution<std::chrono::steady_clock>();
    // Difficult to test with a real clock which will vary based on environment
    // system load etc, just perform some basic sanity checks.

    // Expect to have a non-zero resolution.
    EXPECT_NE(0, result.count());

    fmt::print("estimateClockResolution(steady_clock): {}\n", result);
}

TEST(EstimateClockResolution, CoarseSteadyClock) {
    auto result =
            cb::estimateClockResolution<folly::chrono::coarse_steady_clock>();
    // Difficult to test with a real clock which will vary based on environment
    // system load etc, just perform some basic sanity checks.

    // Expect to have a non-zero resolution.
    EXPECT_NE(0, result.count());

    fmt::print("estimateClockResolution(coarse_steady_clock): {}\n",
               result);
}
