/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "relaxed_atomic.h"

#include <platform/cb_arena_malloc.h>
#include <platform/cb_malloc.h>

#include <folly/portability/GTest.h>

#include <vector>

#if defined(HAVE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#endif

class ArenaMalloc : public ::testing::Test {
protected:
    ArenaMalloc() {
        // Disable tcache as it messes up accurate stats tracking
        // (small allocations can be serviced from tcache and don't
        // show up in stats).
        initialTCacheState = cb::ArenaMalloc::setTCacheEnabled(false);
        cb::ArenaMalloc::switchFromClient();
    }
    ~ArenaMalloc() override {
        cb::ArenaMalloc::setTCacheEnabled(initialTCacheState);
        cb::ArenaMalloc::switchFromClient();
    }

private:
    bool initialTCacheState;
};

TEST_F(ArenaMalloc, cb_malloc_is_using_arenas) {
    EXPECT_TRUE(cb_malloc_is_using_arenas());
}

TEST_F(ArenaMalloc, fragmentation) {
    cb::FragmentationStats stats{2, 200};
    EXPECT_EQ(99, stats.getFragmentationPerc());
}

// Check that allocations made without an explicit client selected are accounted
// in the Global arena.
TEST_F(ArenaMalloc, GlobalArena) {
    auto initialAlloc = cb::ArenaMalloc::getGlobalAllocated();
    auto* p = cb_malloc(1);
    EXPECT_LT(initialAlloc, cb::ArenaMalloc::getGlobalAllocated());
    cb_free(p);
    EXPECT_EQ(initialAlloc, cb::ArenaMalloc::getGlobalAllocated());
}

TEST_F(ArenaMalloc, basicUsage) {
    // JEMalloc will 'lazy' deallocate, so thread cache should be off, with
    // thread caching on we would observe larger allocations than we requested
    // as the tcache fills with big chunks and we may not see the affect of
    // deallocation as the cache will dealloc but not return memory to the arena
    auto client = cb::ArenaMalloc::registerClient(false);

    auto sz1 = cb::ArenaMalloc::getPreciseAllocated(client);
    EXPECT_EQ(0, sz1);

    // 1) Track an allocation
    cb::ArenaMalloc::switchToClient(client);
    auto p = cb_malloc(4096);
    cb::ArenaMalloc::switchFromClient();

    auto sz2 = cb::ArenaMalloc::getPreciseAllocated(client);
    EXPECT_LE(sz1 + 4096, sz2);

    // 2) Allocation outside of switchTo/From not accounted
    auto p2 = cb_malloc(4096);
    EXPECT_EQ(sz2, cb::ArenaMalloc::getPreciseAllocated(client));

    // 3) Track deallocation
    cb::ArenaMalloc::switchToClient(client);
    cb_free(p);
    cb::ArenaMalloc::switchFromClient();
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));
    cb_free(p2);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    cb::ArenaMalloc::unregisterClient(client);
}

TEST_F(ArenaMalloc, threadsRegister) {
    cb::ArenaMallocClient c1, c2;
    std::thread a([&c1]() { c1 = cb::ArenaMalloc::registerClient(); });
    std::thread b([&c2]() { c2 = cb::ArenaMalloc::registerClient(); });
    a.join();
    b.join();
    EXPECT_NE(c1.index, c2.index);
    cb::ArenaMalloc::unregisterClient(c1);
    cb::ArenaMalloc::unregisterClient(c2);
}

/**
 * Test the maximum supported number of arenas.
 */
TEST_F(ArenaMalloc, Limits) {
    // Create vectors to hold each client and the memory each allocated.
    // Pre-reserved to avoid any reallocations during the test which could
    // perturb the specific allocations we want to measure.
    std::vector<cb::ArenaMallocClient> clients(cb::ArenaMallocMaxClients);
    std::vector<void*> allocations;
    allocations.reserve(clients.size());

    // Register maximum number of clients, checking they all start with zero
    // memory allocated.
    for (auto& c : clients) {
        c = cb::ArenaMalloc::registerClient(false);
        EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(c));
    }

    // Attempts to register any more should fail.
    EXPECT_THROW(cb::ArenaMalloc::registerClient(false), std::runtime_error);

    // Allocate differing amounts of memory to each client, then check each
    // reports the correct amount.
    auto sizeForClient = [](const cb::ArenaMallocClient& c) {
        return 32 * (c.index + 1);
    };
    for (auto& c : clients) {
        cb::ArenaMalloc::switchToClient(c);
        allocations.push_back(cb_malloc(sizeForClient(c)));
        cb::ArenaMalloc::switchFromClient();
    }
    for (auto& c : clients) {
        // Cannot accurately predict precisly how much memory will be allocated
        // as allocator may round up, or add extra memory for overhead.
        // As such, just check that we allocated within the range
        // [requested, requested * 2]
        auto allocated = cb::ArenaMalloc::getPreciseAllocated(c);
        EXPECT_LE(sizeForClient(c), allocated);
        EXPECT_LT(allocated, sizeForClient(c) * 2);
    }

    // Check memory returns to zero after all memory freed.
    for (auto& c : clients) {
        cb::ArenaMalloc::switchToClient(c);
        cb_free(allocations.at(c.index));
        cb::ArenaMalloc::switchFromClient();
    }
    for (auto& c : clients) {
        EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(c))
                << "for client index:" << int(c.index);
    }

    // Cleanup
    for (auto& c : clients) {
        cb::ArenaMalloc::unregisterClient(c);
    }
}

#if defined(HAVE_JEMALLOC)
#if defined(WIN32)
// MB-38422: je_malloc_conf isn't being picked up on windows so skip for now.
TEST_F(ArenaMalloc, DISABLED_CheckCustomConfiguration) {
#else
TEST_F(ArenaMalloc, CheckCustomConfiguration) {
#endif
    // Check that the configuration we set in je_malloc_conf has taken affect
    // call je_malloc stat functions to query the config.
    unsigned narenas = 0;
    size_t sz = sizeof(narenas);
    EXPECT_EQ(0, je_mallctl("opt.narenas", &narenas, &sz, nullptr, 0));
    EXPECT_EQ(1, narenas);

    bool prof = false;
    sz = sizeof(prof);
    EXPECT_EQ(0, je_mallctl("opt.prof", &prof, &sz, nullptr, 0));
    EXPECT_TRUE(prof);
}
#endif
