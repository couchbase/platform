/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
