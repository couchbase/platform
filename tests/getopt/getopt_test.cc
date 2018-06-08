/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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

/*
 * Unit tests for the getopt shim implementation of getopt / getopt_long,
 * as required by Win32 which doesn't have <getopt.h>
 */

#include <getopt.h>
#include <gtest/gtest.h>
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

