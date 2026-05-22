/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/io_hint.h>

using namespace cb::io;

class IoHintFormatAsTest : public ::testing::Test {};

class IoHintParseTest : public ::testing::Test {};

class IoHintJsonTest : public ::testing::Test {};

class IoHintRoundTripTest : public ::testing::Test {};

// Tests for format_as
TEST_F(IoHintFormatAsTest, FormatAsNormal) {
    EXPECT_EQ(format_as(IoHint::Normal), "normal");
}

TEST_F(IoHintFormatAsTest, FormatAsSequential) {
    EXPECT_EQ(format_as(IoHint::Sequential), "sequential");
}

TEST_F(IoHintFormatAsTest, FormatAsRandom) {
    EXPECT_EQ(format_as(IoHint::Random), "random");
}

TEST_F(IoHintFormatAsTest, FormatAsNoReuse) {
    EXPECT_EQ(format_as(IoHint::NoReuse), "no-reuse");
}

TEST_F(IoHintFormatAsTest, FormatAsWillNeed) {
    EXPECT_EQ(format_as(IoHint::WillNeed), "will-need");
}

TEST_F(IoHintFormatAsTest, FormatAsDontNeed) {
    EXPECT_EQ(format_as(IoHint::DontNeed), "dont-need");
}

// Tests for parseIoHint - valid inputs
TEST_F(IoHintParseTest, ParseNormal) {
    auto result = parse_io_hint("normal");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::Normal);
}

TEST_F(IoHintParseTest, ParseSequential) {
    auto result = parse_io_hint("sequential");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::Sequential);
}

TEST_F(IoHintParseTest, ParseRandom) {
    auto result = parse_io_hint("random");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::Random);
}

TEST_F(IoHintParseTest, ParseNoReuse) {
    auto result = parse_io_hint("no-reuse");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::NoReuse);
}

TEST_F(IoHintParseTest, ParseWillNeed) {
    auto result = parse_io_hint("will-need");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::WillNeed);
}

TEST_F(IoHintParseTest, ParseDontNeed) {
    auto result = parse_io_hint("dont-need");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), IoHint::DontNeed);
}

// Tests for parseIoHint - invalid inputs
TEST_F(IoHintParseTest, ParseInvalidString) {
    auto result = parse_io_hint("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST_F(IoHintParseTest, ParseEmptyString) {
    auto result = parse_io_hint("");
    EXPECT_FALSE(result.has_value());
}

TEST_F(IoHintParseTest, ParseWithUnderscore) {
    auto result = parse_io_hint("no_reuse");
    EXPECT_FALSE(result.has_value());
}

TEST_F(IoHintParseTest, ParseCaseSensitive) {
    auto result = parse_io_hint("NORMAL");
    EXPECT_FALSE(result.has_value());
}

TEST_F(IoHintParseTest, ParsePartialMatch) {
    auto result = parse_io_hint("no");
    EXPECT_FALSE(result.has_value());
}

// Tests for to_json
TEST_F(IoHintJsonTest, ToJsonNormal) {
    nlohmann::json json;
    to_json(json, IoHint::Normal);
    EXPECT_EQ(json.get<std::string>(), "normal");
}

TEST_F(IoHintJsonTest, ToJsonSequential) {
    nlohmann::json json;
    to_json(json, IoHint::Sequential);
    EXPECT_EQ(json.get<std::string>(), "sequential");
}

TEST_F(IoHintJsonTest, ToJsonRandom) {
    nlohmann::json json;
    to_json(json, IoHint::Random);
    EXPECT_EQ(json.get<std::string>(), "random");
}

TEST_F(IoHintJsonTest, ToJsonNoReuse) {
    nlohmann::json json;
    to_json(json, IoHint::NoReuse);
    EXPECT_EQ(json.get<std::string>(), "no-reuse");
}

TEST_F(IoHintJsonTest, ToJsonWillNeed) {
    nlohmann::json json;
    to_json(json, IoHint::WillNeed);
    EXPECT_EQ(json.get<std::string>(), "will-need");
}

TEST_F(IoHintJsonTest, ToJsonDontNeed) {
    nlohmann::json json;
    to_json(json, IoHint::DontNeed);
    EXPECT_EQ(json.get<std::string>(), "dont-need");
}

// Tests for from_json - valid inputs
TEST_F(IoHintJsonTest, FromJsonNormal) {
    nlohmann::json json = "normal";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Normal);
}

TEST_F(IoHintJsonTest, FromJsonSequential) {
    nlohmann::json json = "sequential";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Sequential);
}

TEST_F(IoHintJsonTest, FromJsonRandom) {
    nlohmann::json json = "random";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Random);
}

TEST_F(IoHintJsonTest, FromJsonNoReuse) {
    nlohmann::json json = "no-reuse";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::NoReuse);
}

TEST_F(IoHintJsonTest, FromJsonWillNeed) {
    nlohmann::json json = "will-need";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::WillNeed);
}

TEST_F(IoHintJsonTest, FromJsonDontNeed) {
    nlohmann::json json = "dont-need";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::DontNeed);
}

// Tests for from_json - invalid inputs (should fall back to Normal)
TEST_F(IoHintJsonTest, FromJsonInvalidStringFallback) {
    nlohmann::json json = "invalid";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Normal);
}

TEST_F(IoHintJsonTest, FromJsonEmptyStringFallback) {
    nlohmann::json json = "";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Normal);
}

TEST_F(IoHintJsonTest, FromJsonUnderscoreFallback) {
    nlohmann::json json = "no_reuse";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Normal);
}

TEST_F(IoHintJsonTest, FromJsonUppercaseFallback) {
    nlohmann::json json = "NORMAL";
    IoHint hint;
    from_json(json, hint);
    EXPECT_EQ(hint, IoHint::Normal);
}

// Round-trip serialization tests
TEST_F(IoHintRoundTripTest, RoundTripNormal) {
    nlohmann::json json;
    IoHint original = IoHint::Normal;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

TEST_F(IoHintRoundTripTest, RoundTripSequential) {
    nlohmann::json json;
    IoHint original = IoHint::Sequential;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

TEST_F(IoHintRoundTripTest, RoundTripRandom) {
    nlohmann::json json;
    IoHint original = IoHint::Random;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

TEST_F(IoHintRoundTripTest, RoundTripNoReuse) {
    nlohmann::json json;
    IoHint original = IoHint::NoReuse;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

TEST_F(IoHintRoundTripTest, RoundTripWillNeed) {
    nlohmann::json json;
    IoHint original = IoHint::WillNeed;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

TEST_F(IoHintRoundTripTest, RoundTripDontNeed) {
    nlohmann::json json;
    IoHint original = IoHint::DontNeed;
    IoHint result;

    to_json(json, original);
    from_json(json, result);

    EXPECT_EQ(result, original);
}

// Test all enum values in a loop for comprehensive coverage
TEST_F(IoHintRoundTripTest, AllEnumValuesRoundTrip) {
    const std::vector<IoHint> hints = {{IoHint::Normal,
                                        IoHint::Sequential,
                                        IoHint::Random,
                                        IoHint::NoReuse,
                                        IoHint::WillNeed,
                                        IoHint::DontNeed}};

    for (auto original : hints) {
        nlohmann::json json;
        IoHint result;

        to_json(json, original);
        from_json(json, result);

        EXPECT_EQ(result, original)
                << "Round-trip failed for " << format_as(original);
    }
}

// Test consistency between parseIoHint and format_as
TEST_F(IoHintRoundTripTest, FormatParseConsistency) {
    const std::vector<IoHint> hints = {{IoHint::Normal,
                                        IoHint::Sequential,
                                        IoHint::Random,
                                        IoHint::NoReuse,
                                        IoHint::WillNeed,
                                        IoHint::DontNeed}};

    for (auto original : hints) {
        std::string_view formatted = format_as(original);
        auto parsed = parse_io_hint(formatted);

        ASSERT_TRUE(parsed.has_value())
                << "Failed to parse format_as result: " << formatted;
        EXPECT_EQ(parsed.value(), original);
    }
}
