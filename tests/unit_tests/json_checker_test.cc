/*
 *     Copyright 2014-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <JSON_checker.h>
#include <folly/portability/GTest.h>
#include <platform/string_hex.h>
#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class DeprecatedInterfaceValidatorTest : public ::testing::Test {
};

#define CHECK_JSON(X) checkUTF8JSON((const unsigned char *)X, sizeof(X) - 1)

TEST_F(DeprecatedInterfaceValidatorTest, SimpleJsonChecksOk) {
    EXPECT_TRUE(CHECK_JSON("{\"test\": 12}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, DeepJsonChecksOk) {
    EXPECT_TRUE(CHECK_JSON(
              "{\"test\": [[[[[[[[[[[[[[[[[[[[[[12]]]]]]]]]]]]]]]]]]]]]]}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BadDeepJsonIsNotOK) {
    EXPECT_FALSE(CHECK_JSON(
                 "{\"test\": [[[[[[[[[[[[[[[[[[[[[[12]]]]]]]]]]]]]]]]]]]]]]]]}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BadJsonStartingWithBraceIsNotOk) {
    EXPECT_FALSE(CHECK_JSON("{bad stuff}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BareValuesAreOk) {
    EXPECT_TRUE(CHECK_JSON("null"));
}

TEST_F(DeprecatedInterfaceValidatorTest, BareNumbersAreOk) {
    EXPECT_TRUE(CHECK_JSON("99"));
}
TEST_F(DeprecatedInterfaceValidatorTest, BadUtf8IsNotOk) {
    EXPECT_FALSE(CHECK_JSON("{\"test\xFF\": 12}"));
}

TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', 0xff, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk2) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', 'a', 0xff, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}

TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk3) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', '1', '2', 0xfe, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk4) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'"', '1', '2', 0xfd, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}
TEST_F(DeprecatedInterfaceValidatorTest, MB15778BadUtf8IsNotOk5) {
    // MB-15778: Regression test for memory leaks.
    unsigned char mb15778[] = {'{', '"', 'k', '"', ':', '"', 0xfc, 0};
    EXPECT_FALSE(CHECK_JSON(mb15778));
}

class ValidatorTest : public ::testing::Test,
                      public ::testing::WithParamInterface<bool> {
public:
    void SetUp() override {
        validator = std::make_unique<JSON_checker::Validator>(GetParam());
    }

protected:
    std::unique_ptr<JSON_checker::Validator> validator;
};

TEST_P(ValidatorTest, SimpleValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    EXPECT_TRUE(validator->validate(value1, sizeof(value1) - 1));
    EXPECT_FALSE(validator->validate(value1, sizeof(value1) - 3));
    EXPECT_TRUE(validator->validate(value1, sizeof(value1) - 1));
}

TEST_P(ValidatorTest, StringLengthTest) {
    for (int i = 0; i < 50; i++) {
        std::string json = R"({"test": ")" + std::string(i, 'x') + "\"}";
        EXPECT_TRUE(validator->validate(json)) << "Failed: " << json;
    }
}

TEST_P(ValidatorTest, ByteArrayValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    std::vector<uint8_t> data;
    data.resize(sizeof(value1) - 1);
    memcpy(data.data(), value1, sizeof(value1) - 1);
    EXPECT_TRUE(validator->validate(data));
    data.resize(data.size() - 1);
    EXPECT_FALSE(validator->validate(data));
    data.emplace_back('}');
    EXPECT_TRUE(validator->validate(data));
}

TEST_P(ValidatorTest, StringValidatorTest) {
    std::string value("{\"test\": 12}");
    EXPECT_TRUE(validator->validate(value));
    value.append("}");
    EXPECT_FALSE(validator->validate(value));
    value.resize(value.length() - 1);
    EXPECT_TRUE(validator->validate(value));
}

TEST_P(ValidatorTest, NumberExponentValidatorTest) {
    EXPECT_TRUE(validator->validate("0e5"));
    EXPECT_TRUE(validator->validate("0E5"));
    EXPECT_TRUE(validator->validate("0.00e5"));
}

TEST_P(ValidatorTest, ControlCharactersTest) {
    // Code points up to 0x1f are all control characters which are illegal.
    for (char i = 0; i <= 0x1f; i++) {
        // Sufficiently large such that vectorisation kicks in
        std::string s = "\"Illegal code point>>";
        s.push_back(i);
        s += "<<inserted into a string\"";

        EXPECT_FALSE(validator->validate(s))
                << "Code point 0x" << std::hex << (int)i << std::dec;
    }
}

TEST_P(ValidatorTest, EquivalenceTest) {
    auto otherValidator =
            std::make_unique<JSON_checker::Validator>(!GetParam());

    for (int i = std::numeric_limits<char>::min();
         i <= std::numeric_limits<char>::max();
         i++) {
        // Sufficiently large such that vectorisation kicks in
        std::string s = "\"Test code>>";
        s.push_back((char)i);
        s += "<<in a string\"";

        EXPECT_EQ(validator->validate(s), otherValidator->validate(s))
                << "Validators do not agree when parsing 0x" << std::hex
                << (int)i << std::dec;
    }
}

/// Wrap the provided bytes in a JSON string sufficiently large such that
/// vectorisation kicks in
static std::string makeString(std::string_view bytes) {
    return "\"Test code>>" + std::string(bytes) + "<<in a string\"";
}

/// The UTF-8 encoding must be strictly validated (RFC 3629) by both
/// implementations
TEST_P(ValidatorTest, Utf8Test) {
    const std::vector<std::string> valid = {"\x7f", // U+007F
                                            "\xc2\x80", // U+0080
                                            "\xc3\xa9", // U+00E9
                                            "\xdf\xbf", // U+07FF
                                            "\xe0\xa0\x80", // U+0800
                                            "\xe2\x82\xac", // U+20AC
                                            "\xed\x9f\xbf", // U+D7FF
                                            "\xee\x80\x80", // U+E000
                                            "\xef\xbf\xbf", // U+FFFF
                                            "\xf0\x90\x80\x80", // U+10000
                                            "\xf0\x9f\x98\x80", // U+1F600
                                            "\xf4\x8f\xbf\xbf"}; // U+10FFFF
    for (const auto& bytes : valid) {
        EXPECT_TRUE(validator->validate(makeString(bytes)))
                << cb::hex_encode(bytes);
    }

    const std::vector<std::string> invalid = {
            "\x80", // Unexpected continuation byte
            "\xbf", // Unexpected continuation byte
            "\xc0\x80", // Overlong U+0000
            "\xc1\xbf", // Overlong U+007F
            "\xe0\x80\x80", // Overlong U+0000
            "\xe0\x9f\xbf", // Overlong U+07FF
            "\xed\xa0\x80", // Surrogate U+D800
            "\xed\xbf\xbf", // Surrogate U+DFFF
            "\xf0\x80\x80\x80", // Overlong U+0000
            "\xf0\x8f\xbf\xbf", // Overlong U+FFFF
            "\xf4\x90\x80\x80", // U+110000
            "\xf5\x80\x80\x80", // Above U+10FFFF
            "\xf7\xbf\xbf\xbf", // Above U+10FFFF
            "\xf8\x88\x80\x80\x80", // 5 byte sequence
            "\xfc\x84\x80\x80\x80\x80", // 6 byte sequence
            "\xfe",
            "\xff",
            "\xc3", // Truncated
            "\xe2\x82", // Truncated
            "\xf0\x9f\x98", // Truncated
            "\xc3\x28", // Invalid continuation byte
            "\xe2\x28\xac", // Invalid continuation byte
            "\xe2\x82\x28", // Invalid continuation byte
            "\xc3\xc3\xa9", // Lead byte inside of a sequence
            "\xe2\x82\xac\xac"}; // Unexpected continuation byte
    for (const auto& bytes : invalid) {
        EXPECT_FALSE(validator->validate(makeString(bytes)))
                << cb::hex_encode(bytes);
    }
}

/// The scalar and vectorized implementations must agree on the UTF-8
/// encoding of all (one and two byte) sequences, and of all lead bytes of
/// three and four byte sequences combined with all second bytes and the
/// boundary values of the following bytes
TEST_P(ValidatorTest, Utf8EquivalenceTest) {
    auto otherValidator =
            std::make_unique<JSON_checker::Validator>(!GetParam());
    auto check = [&](std::string_view bytes) {
        const auto s = makeString(bytes);
        EXPECT_EQ(validator->validate(s), otherValidator->validate(s))
                << "Validators do not agree when parsing "
                << cb::hex_encode(bytes);
    };

    for (int ii = 0x80; ii <= 0xff; ++ii) {
        for (int jj = 0; jj <= 0xff; ++jj) {
            check(std::string{char(ii), char(jj)});
        }
    }

    const std::array<char, 4> boundaries = {'\x7f', '\x80', '\xbf', '\xc0'};
    for (int ii = 0xe0; ii <= 0xff; ++ii) {
        for (int jj = 0; jj <= 0xff; ++jj) {
            for (const auto third : boundaries) {
                check(std::string{char(ii), char(jj), third});
                for (const auto fourth : boundaries) {
                    check(std::string{char(ii), char(jj), third, fourth});
                }
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(JSON_checker,
                         ValidatorTest,
                         ::testing::Bool(),
                         ::testing::PrintToStringParamName());
