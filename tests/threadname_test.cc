/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
#include "config.h"
#include <platform/platform.h>
#include <platform/cbassert.h>
#include <cstring>
#include <cerrno>

#include <gtest/gtest.h>

TEST(ThreadnameTest, ThreadName) {
    if (!is_thread_name_supported()) {
        return;
    }

    EXPECT_EQ(0, cb_set_thread_name("test"));
    char buffer[80];
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));

    memset(buffer, 'a', sizeof(buffer));
    buffer[79] ='\0';
    EXPECT_EQ(1, cb_set_thread_name(buffer)) << " errno " << errno << " "
                                             << strerror(errno);

    memset(buffer, 0, sizeof(buffer));

    // Check that a failing set thread name didn't mess up the value
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));
}
