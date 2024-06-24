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
