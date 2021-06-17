/*
 *     Copyright 2014-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <platform/random.h>
#include <array>

TEST(RandomGenerator, RandomTest) {
    cb::RandomGenerator r1;
    cb::RandomGenerator r2;

    uint64_t v1 = r1.next();
    uint64_t v2 = r2.next();
    EXPECT_NE(v1, v2) << "I did not expect the random generators to return the"
                      << " same value";

    std::array<char, 1024> buffer;
    std::fill(buffer.begin(), buffer.end(), 0);
    if (r1.getBytes(buffer.data(), buffer.size())) {
        size_t ii;

        for (ii = 0; ii < buffer.size(); ++ii) {
            if (buffer[ii] != 0) {
                break;
            }
        }

        EXPECT_NE(buffer.size(), ii) << "I got 1k of 0 (or it isn't working)";
    }
}
