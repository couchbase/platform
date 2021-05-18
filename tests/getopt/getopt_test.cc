/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
 * Unit tests for the getopt shim implementation of getopt / getopt_long,
 * as required by Win32 which doesn't have <getopt.h>
 */

#include <folly/portability/GTest.h>
#include <getopt.h>
#include <array>

// Under Win32 we alias cb::getopt() to getopt(), as Win32 doesn't have
// getopt(). Test that a second call to getopt() succeeds as long as it is
// reset via optind.
TEST(SystemGetoptTest, TestMultipleCalls) {
    std::array<const char*, 2> opts{{"program", "-a"}};
    const auto argc = opts.size();
    char** argv = const_cast<char**>(opts.data());

    // Call getopt once; advancing it's state.
    ASSERT_EQ('a', getopt(argc, argv, "ab"));
    ASSERT_EQ(-1, getopt(argc, argv, "ab"));

    // Reset optind, check that this allows us to parse a second time.
    optind = 1;
    EXPECT_EQ('a', getopt(argc, argv, "ab"));
    EXPECT_EQ(-1, getopt(argc, argv, "ab"));
}

