/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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
#include <cJSON_utils.h>
#include <gtest/gtest.h>
#include <gsl/gsl>

TEST(cJSON, ToStringInvalidArguments) {
    unique_cJSON_ptr ptr;
    EXPECT_THROW(to_string(ptr.get()), std::invalid_argument);
    EXPECT_THROW(to_string(ptr), std::invalid_argument);
}

TEST(cJSON, ToStringDefaultArg) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddStringToObject(ptr.get(), "foo", "bar");

    std::string expected("{\n\t\"foo\":\t\"bar\"\n}");
    EXPECT_EQ(expected, to_string(ptr));
    EXPECT_EQ(expected, to_string(ptr.get()));
}

TEST(cJSON, ToStringFormatted) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddStringToObject(ptr.get(), "foo", "bar");

    std::string expected("{\n\t\"foo\":\t\"bar\"\n}");
    EXPECT_EQ(expected, to_string(ptr, true));
    EXPECT_EQ(expected, to_string(ptr.get(), true));
}

TEST(cJSON, ToStringUnformatted) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddStringToObject(ptr.get(), "foo", "bar");

    std::string expected("{\"foo\":\"bar\"}");
    EXPECT_EQ(expected, to_string(ptr, false));
    EXPECT_EQ(expected, to_string(ptr.get(), false));
}

TEST(cJSON, ToStringEmptyObject) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    EXPECT_EQ("{\n}", to_string(ptr));
}

TEST(cJSON, ToStringEmptyObjectUnFormatted) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    EXPECT_EQ("{}", to_string(ptr, false));
}

TEST(cJSON, ToStringEmptyObjectAsField) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddItemToObject(ptr.get(), "foo", cJSON_CreateObject());
    EXPECT_EQ("{\n\t\"foo\":\t{\n}\n}", to_string(ptr));
}

TEST(cJSON, ToStringEmptyObjectAsFieldUnformatted) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddItemToObject(ptr.get(), "foo", cJSON_CreateObject());
    EXPECT_EQ("{\"foo\":{}}", to_string(ptr, false));
}

TEST(cJSON, ToStringEmptyArray) {
    unique_cJSON_ptr ptr(cJSON_CreateArray());
    EXPECT_EQ("[\n]", to_string(ptr));
}

TEST(cJSON, ToStringEmptyArrayUnFormatted) {
    unique_cJSON_ptr ptr(cJSON_CreateArray());
    EXPECT_EQ("[]", to_string(ptr, false));
}

TEST(cJSON, ToStringEmptyArrayAsField) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddItemToObject(ptr.get(), "foo", cJSON_CreateArray());
    EXPECT_EQ("{\n\t\"foo\":\t[\n]\n}", to_string(ptr));
}

TEST(cJSON, ToStringEmptyArrayAsFieldUnformatted) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddItemToObject(ptr.get(), "foo", cJSON_CreateArray());
    EXPECT_EQ("{\"foo\":[]}", to_string(ptr, false));
}

TEST(cJSON, ToStringAddIntegerToObject) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddIntegerToObject(ptr.get(), "foo", 0xdeadbeef);
    EXPECT_EQ("{\"foo\":3735928559}", to_string(ptr, false));
}

TEST(cJSON, ToStringAddInteger64ToObject_Safe) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddInteger64ToObject(ptr.get(), "foo", 0xdeadbeef);
    EXPECT_EQ("{\"foo\":3735928559}", to_string(ptr, false));
}

TEST(cJSON, ToStringAddInteger64ToObject_Narrowing) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddInteger64ToObject(ptr.get(), "foo", 0xdeadbeefdeadbeef);
    double a = gsl::narrow_cast<double>(0xdeadbeefdeadbeef);
    std::stringstream expected;
    expected << "{\"foo\":" << std::fixed << std::setprecision(0) << a << "}";
    EXPECT_EQ(expected.str(), to_string(ptr, false));
}

TEST(cJSON, ToStringAddStringifiedInteger_Unsigned) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddStringifiedIntegerToObject(ptr.get(), "foo", uint64_t(0xdeadbeef));
    EXPECT_EQ("{\"foo\":\"3735928559\"}", to_string(ptr, false));
}

TEST(cJSON, ToStringAddStringifiedInteger_Signed) {
    unique_cJSON_ptr ptr(cJSON_CreateObject());
    cJSON_AddStringifiedSignedIntegerToObject(
            ptr.get(), "foo", int64_t(0xdeadbeef));
    EXPECT_EQ("{\"foo\":\"3735928559\"}", to_string(ptr, false));
    cJSON_AddStringifiedSignedIntegerToObject(ptr.get(), "bar", int64_t(-1));
    EXPECT_EQ("{\"foo\":\"3735928559\",\"bar\":\"-1\"}", to_string(ptr, false));
}
