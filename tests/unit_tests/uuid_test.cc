/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>

#include <platform/uuid.h>

TEST(UUID, to_string) {
    EXPECT_EQ(36u, to_string(cb::uuid::random()).size());
}

TEST(UUID, from_string) {
    auto out = cb::uuid::random();

    // Verify that a roundtrip conversion works
    EXPECT_EQ(out, cb::uuid::from_string(to_string(out)));

    // Verify that it throws for wrong length
    EXPECT_THROW(cb::uuid::from_string(""), std::invalid_argument);

    // Verify that it throws for wrongly placed hyphen
    EXPECT_THROW(cb::uuid::from_string("00000000-0000-000000000-000000000000"),
                 std::invalid_argument);

    // Verify that it throws for invalid conversion from hex
    EXPECT_THROW(cb::uuid::from_string("00000000-0000-0000-/000-000000000000"),
                 std::invalid_argument);

    // Verify it doesn't have the pitfalls of stoul/strtoul
    EXPECT_THROW(cb::uuid::from_string("0X0X0X0X-0X0X-0X0X-0X0X-0X0X0X0X0X0X"),
                 std::invalid_argument);
}
