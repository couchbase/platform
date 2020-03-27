/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2020 Couchbase, Inc
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

#include <folly/portability/GTest.h>
#include <platform/optional.h>
#include <optional>

TEST(OptionalTest, ToStringEmptyInt) {
    auto empty = std::optional<int>{};
    EXPECT_EQ("none", to_string_or_none(empty));
}

TEST(OptionalTest, ToStringPresentInt) {
    auto present = std::make_optional(123);
    EXPECT_EQ("123", to_string_or_none(present));
}

struct Custom {
    int x;
};
std::string to_string(const Custom& custom) {
    return std::to_string(custom.x);
}

TEST(OptionalTest, ToStringPresentUserDefinedToString) {
    auto present = std::make_optional(Custom{123});
    EXPECT_EQ("123", to_string_or_none(present));
}
