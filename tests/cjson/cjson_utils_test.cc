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
