/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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

#include <gtest/gtest.h>
#include <platform/string.h>

TEST(Hex, InputStringTooLong) {
    std::stringstream ss;
    ss << std::hex << uint64_t(-1);
    EXPECT_EQ(-1ULL, cb::from_hex(ss.str()));
    // Make sure it won't fit in there
    ss << "0";
    EXPECT_THROW(cb::from_hex(ss.str()), std::overflow_error);
}

TEST(Hex, InputStringData) {
    const std::string value{"0123456789abcdef"};
    const uint64_t expected{0x0123456789abcdefULL};
    EXPECT_EQ(expected, cb::from_hex(value));
}
