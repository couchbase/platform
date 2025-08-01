/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>

#include <platform/json_log.h>
#include <platform/json_log_conversions.h>
#include <optional>
#include <stdexcept>

TEST(JsonLog, Basic) {
    using cb::logger::Json;

    Json x{{"foo", "bar"}};
    EXPECT_EQ(R"({"foo":"bar"})", x.dump());

    Json xCopyConstructor(x);
    Json xMoveConstructor(std::move(x));
    Json xAssignment;

    EXPECT_EQ(R"({"foo":"bar"})", xCopyConstructor.dump());
    EXPECT_EQ(R"({"foo":"bar"})", xMoveConstructor.dump());

    xAssignment = xCopyConstructor;
    EXPECT_EQ(R"({"foo":"bar"})", xAssignment.dump());

    xAssignment = std::move(xCopyConstructor);
    EXPECT_EQ(R"({"foo":"bar"})", xMoveConstructor.dump());
}

enum class Color { Red, Green, Blue };

std::string_view format_as(Color c) {
    switch (c) {
    case Color::Red:
        return "Red";
    case Color::Green:
        return "Green";
    case Color::Blue:
        return "Blue";
    }
    throw std::invalid_argument("c");
}

TEST(JsonLog, Enums) {
    using cb::logger::Json;

    EXPECT_EQ("\"Red\"", Json(Color::Red).dump());
}

enum class Flags { A, B, C };

std::string_view format_as(Flags flags) {
    return "format_as";
}

void to_json(nlohmann::json& j, Flags flags) {
    j = "to_json";
}

TEST(JsonLog, EnumsWithToJson) {
    using cb::logger::Json;

    EXPECT_EQ("\"to_json\"", Json(Flags::A).dump());
}

TEST(JsonLog, Parse) {
    using cb::logger::Json;
    Json j = Json::parse("123");
    EXPECT_TRUE(j.is_number());
    EXPECT_EQ(123, j.template get<int>());
}

TEST(JsonLog, Compose) {
    using cb::logger::Json;

    Json array = Json::array();
    Json string = Json("");

    Json initWithCopy{
            {"array", array},
            {"string", string},
    };

    Json initWithMove{
            {"array", std::move(array)},
            {"string", std::move(string)},
    };
}

TEST(JsonLog, Optional) {
    using cb::logger::Json;

    Json one(std::optional<int>{1});
    Json empty(std::optional<int>{});

    EXPECT_EQ("1", one.dump());
    EXPECT_EQ("null", empty.dump());
}

TEST(JsonLog, FormatNonAscii) {
    using cb::logger::Json;

    // Non-ASCII characters are escaped.
    Json j{{"foo", "ä"}};
    EXPECT_EQ(R"({"foo":"\u00e4"})", fmt::to_string(j));
}

TEST(JsonLog, AllowInvalidUtf8) {
    using cb::logger::Json;

    Json j{{"foo", "\xff"}};
    // Invalid UTF-8 is replaced with the Unicode replacement character and then
    // escaped.
    EXPECT_EQ(R"({"foo":"\ufffd"})", fmt::to_string(j));
}

TEST(JsonLog, FormatDurations) {
    using namespace std::chrono_literals;
    using cb::logger::Json;

    EXPECT_EQ("\"1000 ms\"", Json(1000000000ns).dump());
    EXPECT_EQ("\"1000 ms\"", Json(1000000us).dump());
    EXPECT_EQ("\"1000 ms\"", Json(1000ms).dump());
    EXPECT_EQ("\"1000 ms\"", Json(1s).dump());

    EXPECT_EQ("\"13m:13s\"", Json(13min + 13s + 13ms).dump());
    EXPECT_EQ("\"13m:13s\"", Json(13min + 13s + 13ms).dump());
}
