/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2014 Couchbase, Inc
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

#include "config.h"

#include <gtest/gtest.h>
#include <iostream>
#include <JSON_checker.h>

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

class ValidatorTest : public ::testing::Test {
protected:
    JSON_checker::Validator validator;
};

TEST_F(ValidatorTest, SimpleValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    EXPECT_TRUE(validator.validate(value1, sizeof(value1) - 1));
    EXPECT_FALSE(validator.validate(value1, sizeof(value1) - 3));
    EXPECT_TRUE(validator.validate(value1, sizeof(value1) - 1));
}

TEST_F(ValidatorTest, ByteArrayValidatorTest) {
    const uint8_t value1[] = "{\"test\": 12}";
    std::vector<uint8_t> data;
    data.resize(sizeof(value1) - 1);
    memcpy(data.data(), value1, sizeof(value1) - 1);
    EXPECT_TRUE(validator.validate(data));
    data.resize(data.size() - 1);
    EXPECT_FALSE(validator.validate(data));
    data.emplace_back('}');
    EXPECT_TRUE(validator.validate(data));
}

TEST_F(ValidatorTest, StringValidatorTest) {
    std::string value("{\"test\": 12}");
    EXPECT_TRUE(validator.validate(value));
    value.append("}");
    EXPECT_FALSE(validator.validate(value));
    value.resize(value.length() - 1);
    EXPECT_TRUE(validator.validate(value));
}
