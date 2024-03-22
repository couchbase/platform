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

TEST(JsonLog, Basic) {
    using cb::logger::Json;

    cb::logger::Json x{{"foo", "bar"}};
    EXPECT_EQ(R"({"foo":"bar"})", x.dump());

    cb::logger::Json xCopyConstructor(x);
    cb::logger::Json xMoveConstructor(std::move(x));
    cb::logger::Json xAssignment;

    EXPECT_EQ(R"({"foo":"bar"})", xCopyConstructor.dump());
    EXPECT_EQ(R"({"foo":"bar"})", xMoveConstructor.dump());

    xAssignment = xCopyConstructor;
    EXPECT_EQ(R"({"foo":"bar"})", xAssignment.dump());

    xAssignment = std::move(xCopyConstructor);
    EXPECT_EQ(R"({"foo":"bar"})", xMoveConstructor.dump());
}
