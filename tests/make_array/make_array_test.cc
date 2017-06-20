/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
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
#include <platform/make_array.h>
#include <platform/sized_buffer.h>

TEST(MakeArrayTest, Basic) {
    // Just a simple smoke test to make sure its doing sensible things
    constexpr auto number_array = cb::make_array(1, 2, 3, 4, 5);
    EXPECT_EQ(4, number_array.at(3));
    EXPECT_EQ(5, number_array.size());

    // Check non-primitives are handled nicely
    constexpr auto string_array = cb::make_array(
            "Hello"_ccb, "World!"_ccb, "Couchbase"_ccb, "Rules!"_ccb);
    EXPECT_EQ(cb::const_char_buffer("Couchbase"), string_array.at(2));

    // Check nesting works OK
    constexpr auto nested_array =
            cb::make_array(cb::make_array(1, 2, 3), cb::make_array(4, 5, 6));
    EXPECT_EQ(4, nested_array.at(1).at(0));
}
