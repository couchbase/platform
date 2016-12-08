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
#include <gtest/gtest.h>
#include <platform/cb_malloc.h>
#include <stdio.h>
#include <strings.h>

TEST(AsprintfTest, NoFormatting) {
    char* result = nullptr;
    EXPECT_EQ(6, asprintf(&result, "test 1"));
    EXPECT_EQ(std::string{"test 1"}, result);
    free(result);
}

TEST(AsprintfTest, SingleFormatting) {
    char* result = nullptr;
    EXPECT_EQ(6, asprintf(&result, "test %d", 2));
    EXPECT_EQ(std::string{"test 2"}, result);
    free(result);
}

TEST(AsprintfTest, MultipleFormatting) {
    char* result = nullptr;
    EXPECT_EQ(6, asprintf(&result, "%c%c%c%c %d", 't', 'e', 's', 't', 3));
    EXPECT_EQ(std::string{"test 3"}, result);
    free(result);
}

