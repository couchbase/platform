/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc
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

#include "corestore_test.h"

#include <platform/corestore.h>

#include <atomic>
#include <iostream>

TEST_F(CoreStoreTest, test) {
    CoreStore<std::atomic<uint32_t>> corestore;

    std::cout << corestore.size() << std::endl;

    size_t count = 0;
    for (auto& e : corestore) {
        EXPECT_EQ(0u, e.load());
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
    EXPECT_EQ(1u, count);
}

int ArrayTest::cpuCount;
int ArrayTest::cpuIndex;

TEST_P(ArrayTest, testCoreArraySize) {
    // Check we have the correct number of elements in the coreArray
    EXPECT_EQ(getExpectedArrSizeParam(), corestore->size());
}

TEST_P(ArrayTest, testCoreArrayIndex) {
    // Insert a unique value into each element that we should have created, then
    // check for correctness
    for (int i = 0; i < getExpectedArrSizeParam(); i++) {
        cpuIndex = i;
        corestore->get() = i;
    }

    for (int i = 0; i < getExpectedArrSizeParam(); i++) {
        cpuIndex = i;
        EXPECT_EQ(i, corestore->get());
    }
}

TEST_P(ArrayTest, testIncreaseCpuCount) {
    // Insert a unique value into each element that we should have created
    for (int i = 0; i < getExpectedArrSizeParam(); i++) {
        cpuIndex = i;
        corestore->get() = i;
    }

    // Overrun the bounds of the coreArray, the bitmask should map us correctly
    // to the modulus of the new index value
    for (int i = 0; i <= 4 * getExpectedArrSizeParam(); i++) {
        cpuIndex = i;
        EXPECT_EQ(i % getExpectedArrSizeParam(), corestore->get());
    }
}

INSTANTIATE_TEST_CASE_P(CoreStoreArrayTest,
                        ArrayTest,
                        ::testing::Values(std::make_tuple(1, 1),
                                          std::make_tuple(2, 2),
                                          std::make_tuple(3, 4),
                                          std::make_tuple(4, 4),
                                          std::make_tuple(5, 8),
                                          std::make_tuple(6, 8),
                                          std::make_tuple(7, 8),
                                          std::make_tuple(8, 8)), );
