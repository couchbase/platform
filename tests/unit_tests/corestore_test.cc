/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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

// folly::AccessSpreader::initialize() requires cpuCount is non-zero or it will
// infinite loop. Given initialise() is executed once on startup we must
// initialise cpuCount to a non-zero value.
unsigned ArrayTest::cpuCount = 1;
unsigned ArrayTest::cpuIndex;

size_t ArrayTest::getCpuIndex(size_t c) {
    auto index = folly::AccessSpreader<ManualTag>::current(c);
    return index;
}

int ArrayTest::testingGetCpu(unsigned* cpu,
                             unsigned* node,
                             void* /* unused */) {
    if (cpu) {
        *cpu = ArrayTest::cpuIndex;
    }

    if (node) {
        *node = ArrayTest::cpuIndex;
    }
    // Return value not checked
    return 0;
}

TEST_F(ArrayTest, testCoreArraySize) {
    // Check we have the correct number of elements in the coreArray
    for (int i = 0; i < 200; ++i) {
        cpuCount = i;
        corestore = std::make_unique<
                CoreStore<uint8_t, &getCpuCount, &getCpuIndex>>();
        EXPECT_EQ(getCpuCount(), corestore->size());
    }
}

TEST_F(ArrayTest, testCpuGTCoreArraySize) {
    cpuCount = 4;
    corestore =
            std::make_unique<CoreStore<uint8_t, &getCpuCount, &getCpuIndex>>();
    for (int i = 0; i < 200; ++i) {
        cpuIndex = i;
        EXPECT_EQ(0, corestore->get());
    }
}
