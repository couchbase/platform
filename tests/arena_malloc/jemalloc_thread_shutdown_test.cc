/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <jemalloc/jemalloc.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

/**
 * Allocates the given size and fills with std::rand, then frees the allocation.
 */
static void allocateAndFree(size_t nbytes, bool verbose = false) {
    // Use new and delete which calls into our overrides.
    if (verbose) {
        std::fprintf(stderr, "malloc(%zu)\n", nbytes);
    }
    char* buffer = static_cast<char*>(je_mallocx(nbytes, 0));
    for (size_t i = 0; i < nbytes; i += sizeof(int)) {
        int value = std::rand();
        std::memcpy(buffer + i, &value, sizeof(int));
    }
    if (verbose) {
        std::fprintf(stderr, "...free()\n");
    }
    je_dallocx(buffer, 0);
}

/**
 * Call allocateAndFree() for all jemalloc bin sizes.
 */
static void allocateAndFreeAllBinSizes() {
    unsigned nbins = 0;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    size_t mib[4];

    size_t len = sizeof(nbins);
    je_mallctl("arenas.nbins", &nbins, &len, nullptr, 0);

    size_t mibLen = 4;
    je_mallctlnametomib("arenas.bin.0.size", mib, &mibLen);
    for (unsigned i = 0; i < nbins; i++) {
        mib[2] = i;
        size_t binSize = 0;
        len = sizeof(binSize);
        je_mallctlbymib(mib, mibLen, &binSize, &len, nullptr, 0);

        // We've seen failures only with the 3K bin size allocation, but we
        // try various sizes so that we can continue to detect the issue even
        // if the internal state in jemalloc becomes smaller/larger.
        if (binSize > 16 * 1024) {
            break;
        }

        allocateAndFree(binSize, true);
    }
}

struct AllocateOnExit {
    ~AllocateOnExit() {
        allocateAndFreeAllBinSizes();
    }
};

/**
 * Verify that allocation can be done during thread shutdown.
 */
static void reincarnateThreadStateTest() {
    // Init jemalloc.
    allocateAndFree(8);

    // Run the test 20 times with different random fill sequences.
    for (int i = 1; i <= 20; i++) {
        std::srand(i);
        std::fprintf(stderr, "Seeding std::rand with %d\n", i);
        std::thread t([]() {
            // Init this jemalloc's thread structure.
            allocateAndFree(8);
            thread_local AllocateOnExit obj;
        });
        t.join();
    }
}

int main() {
    reincarnateThreadStateTest();
    std::fprintf(stderr, "Success!\n");
}
