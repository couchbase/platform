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
#include <iostream>
#include <limits>
#include <memory>

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

INSTANTIATE_TEST_SUITE_P(JSON_checker,
                         ValidatorTest,
                         ::testing::Bool(),
                         ::testing::PrintToStringParamName());
