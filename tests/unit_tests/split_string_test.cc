/*
 *    Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/split_string.h>

using namespace std::literals;

/// Verify that an empty input returns an empty vector
TEST(SplitStringTest, EmptyInputData) {
    EXPECT_TRUE(cb::string::split({}).empty());
}

/// Verify that we can split an input string into its space separated parts
TEST(SplitStringTest, SplitDefault) {
    std::vector<std::string_view> blueprint(
            {"Dette"sv, "er"sv, "en"sv, "test"sv});
    auto res = cb::string::split("Dette er en test"sv);
    EXPECT_EQ(blueprint, res);
}

/// Verify that we don't swallow an "empty" slot in the input line
TEST(SplitStringTest, WithEmptyToken) {
    std::vector<std::string_view> blueprint(
            {"Dette"sv, "er"sv, ""sv, "en"sv, "test"sv});
    auto res = cb::string::split("Dette er  en test"sv);
    EXPECT_EQ(blueprint, res);
}

/// Verify that we swallow an "empty" slot in the input line if requested
TEST(SplitStringTest, WithEmptyTokenNotAllowed) {
    std::vector<std::string_view> blueprint(
            {"Dette"sv, "er"sv, "en"sv, "test"sv});
    auto res = cb::string::split("Dette er  en test"sv, ' ', false);
    EXPECT_EQ(blueprint, res);
}

/// Verify that we may use a different separator character
TEST(SplitStringTest, ColonSeparator) {
    std::vector<std::string_view> blueprint({"Dette er en"sv, " test"sv});
    auto res = cb::string::split("Dette er en: test"sv, ':');
    EXPECT_EQ(blueprint, res);
}
