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

#include <folly/concurrency/CacheLocality.h>
#include <folly/portability/GTest.h>

#include <memory>

class CoreStoreTest : public ::testing::Test {
};

/**
 * Test fixture for a CoreStore test that allows us to inject interesting values
 * for cpu count and cpu index at runtime. Parameterized for configuration with
 * different cores and the expected number of array elements that should be
 * allocated.
 */
class ArrayTest : public CoreStoreTest {
public:
    /**
     * Cpu count value that we can modify to determine how large a coreArray we
     * should make in the corestore
     */
    static unsigned cpuCount;

    /**
     * Cpu index value that we can modify to tell corestore/Folly's
     * AccessSpreader which CPU is being used
     */
    static unsigned cpuIndex;

    /**
     * Linux style getCPU func, used instead of the real/VDSO function to inject
     * a specific value to test. Required to use our ManualTag AccessSpreader.
     */
    static int testingGetCpu(unsigned* cpu, unsigned* node, void* /*unused*/);

protected:
    /**
     * Static functions and members so that we can inject interesting values
     * in place of real cpu counts and indexes
     */
    static size_t getCpuCount() {
        return cpuCount;
    };
    static size_t getCpuIndex(size_t);

    /**
     * Unique ptr to our corestore that we want to test so that we can
     * defer initialization until we can set our static cpu count config value
     */
    std::unique_ptr<CoreStore<uint8_t, &getCpuCount, &getCpuIndex>> corestore;
};

/**
 * Ripped this from folly's test class to allow us to manually tell it which CPU
 * it should think we are using.
 */
#ifdef FOLLY_TLS
#define DECLARE_SPREADER_TAG(tag, locality, func)        \
    namespace {                                          \
    template <typename dummy>                            \
    struct tag {};                                       \
    }                                                    \
    namespace folly {                                    \
    template <>                                          \
    const CacheLocality& CacheLocality::system<tag>() {  \
        static auto* inst = new CacheLocality(locality); \
        return *inst;                                    \
    }                                                    \
    template <>                                          \
    Getcpu::Func AccessSpreader<tag>::pickGetcpuFunc() { \
        return func;                                     \
    }                                                    \
    template struct AccessSpreader<tag>;                 \
    }

DECLARE_SPREADER_TAG(ManualTag,
                     CacheLocality::uniform(ArrayTest::cpuCount),
                     ArrayTest::testingGetCpu)
#endif
