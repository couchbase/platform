/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <platform/corestore.h>

#include <gtest/gtest.h>

#include <atomic>
#include <iostream>

class CoreStoreTest : public ::testing::Test {
public:
};

TEST_F(CoreStoreTest, test) {
    CoreStore<std::atomic<uint32_t>> corestore;

    std::cout << corestore.size() << std::endl;

    int count = 0;
    for (auto& e : corestore) {
        EXPECT_EQ(0, e.load());
        count++;
    }
    EXPECT_EQ(corestore.size(), count);
    corestore.get()++;
    // Can't guarantee the next get would hit the same element... so check all
    count = 0;
    for (auto& e : corestore) {
        if (e.load() != 0) {
            count++;
        }
    }

    // Expected on core slot to be non-zero
    EXPECT_EQ(1, count);
}