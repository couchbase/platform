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

#pragma once

#include <platform/corestore.h>

#include <gtest/gtest.h>
#include <platform/make_unique.h>

class CoreStoreTest : public ::testing::Test {
public:
};

/**
 * Test fixture for a CoreStore test that allows us to inject interesting values
 * for cpu count and cpu index at runtime. Parameterized for configuration with
 * different cores and the expected number of array elements that should be
 * allocated.
 */
class ArrayTest : public CoreStoreTest,
                  public ::testing::WithParamInterface<
                          std::tuple</*coreCount*/ uint8_t,
                                     /*expectedArraySize*/ uint8_t>> {
protected:
    void SetUp() override {
        cpuCount = getCpuCountParam();
        corestore = std::make_unique<
                CoreStore<uint8_t, &getCpuCount, &getCpuIndex>>();
    }

    uint8_t getCpuCountParam() {
        return std::get<0>(GetParam());
    }

    uint8_t getExpectedArrSizeParam() {
        return std::get<1>(GetParam());
    }

    /**
     * Static functions and members so that we can inject interesting values
     * in place of real cpu counts and indexes
     */
    static size_t getCpuCount() {
        return cpuCount;
    };
    static size_t getCpuIndex() {
        return cpuIndex;
    };
    static int cpuCount;
    static int cpuIndex;

    /**
     * Unique ptr to our corestore that we want to test so that we can
     * defer initialization until we can set our static cpu count config value
     */
    std::unique_ptr<CoreStore<uint8_t, &getCpuCount, &getCpuIndex>> corestore;
};