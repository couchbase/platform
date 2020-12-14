/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

#include <folly/portability/GTest.h>
#include <platform/checked_snprintf.h>

TEST(checked_snprintf, DestinationNullptr) {
    EXPECT_THROW(checked_snprintf(nullptr, 10, "xyz"), std::invalid_argument);
}

TEST(checked_snprintf, DestinationSize0) {
    char* buf = nullptr;
    buf++;
    EXPECT_THROW(checked_snprintf(buf, 0, "xyz"), std::invalid_argument);
}

TEST(checked_snprintf, FitInBuffer) {
    char buffer[10];
    EXPECT_EQ(4, checked_snprintf(buffer, sizeof(buffer), "test"));
    EXPECT_EQ(std::string("test"), buffer);
}

TEST(checked_snprintf, BufferTooSmall) {
    char buffer[10];
    EXPECT_THROW(checked_snprintf(buffer, sizeof(buffer), "test %s %u",
                                  "with a buffer that is too big", 10),
                 std::overflow_error);
}
