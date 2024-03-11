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

#include <platform/ordered_map.h>

TEST(OrderedMap, Basic) {
    cb::OrderedMap<int, int> o;
    o.emplace(1, 2);
    o.emplace(3, 4);

    EXPECT_NE(o.end(), o.find(1));
    EXPECT_EQ(o.end(), o.find(2));
    EXPECT_NE(o.end(), o.find(3));
    EXPECT_EQ(o.end(), o.find(4));
}
