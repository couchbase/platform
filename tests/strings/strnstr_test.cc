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
#include <platform/platform.h>

TEST(StrnstrTest, EmptyBuffer) {
    std::string source;
    EXPECT_EQ(nullptr, cb::strnstr(source.data(), "", source.length()));
}

TEST(StrnstrTest, NoHit) {
    std::string source{"yo bubbafoo"};
    EXPECT_EQ(nullptr, cb::strnstr(source.data(), "foo", source.length() - 3));
}

TEST(StrnstrTest, MatchEntireString) {
    std::string source{"yo bubba"};
    EXPECT_EQ(intptr_t(source.data()),
              intptr_t(cb::strnstr(source.data(), source.c_str(),
                                   source.length())));
}

TEST(StrnstrTest, HitFirst) {
    std::string source{"yo bubba"};
    EXPECT_EQ(intptr_t(source.data()),
              intptr_t(cb::strnstr(source.data(), "yo", source.length())));
}

TEST(StrnstrTest, HitLastCharacter) {
    std::string source{"yo bubba"};
    EXPECT_EQ(intptr_t(source.data()) + 7,
              intptr_t(cb::strnstr(source.data(), "a", source.length())));
}

TEST(StrnstrTest, HitMiddleCharacter) {
    std::string source{"yo bubba"};
    EXPECT_EQ(intptr_t(source.data()) + 3,
              intptr_t(cb::strnstr(source.data(), "b", source.length())));
}

TEST(StrnstrTest, HitMiddleString) {
    std::string source{"yo bubba"};
    EXPECT_EQ(intptr_t(source.data()) + 5,
              intptr_t(cb::strnstr(source.data(), "bb", source.length())));
}

TEST(StrnstrTest, SpanEndOfString) {
    std::string source{"yo bubbare"};
    EXPECT_EQ(nullptr, cb::strnstr(source.data(), "bare", source.length() - 2));
}

TEST(StrnstrTest, SpanNullTerm) {
    std::string source{"yo bubba"};
    source[2] = '\0';
    EXPECT_EQ(nullptr, cb::strnstr(source.data(), "bubba", source.length()));
}