/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
