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

#include <folly/ScopeGuard.h>
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
    EXPECT_EQ(0.99, stats.getFragmentationRatio());
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
    for (size_t domain = 0; domain < size_t(cb::MemoryDomain::Count);
         domain++) {
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain(domain)));
    }

    // 1) Track an allocation
    cb::ArenaMalloc::switchToClient(client);
    auto p = cb_malloc(4096);
    cb::ArenaMalloc::switchFromClient();

    auto sz2 = cb::ArenaMalloc::getPreciseAllocated(client);
    EXPECT_LE(sz1 + 4096, sz2);
    EXPECT_EQ(cb::ArenaMalloc::getPreciseAllocated(client),
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));

    // 2) Allocation outside of switchTo/From not accounted
    auto p2 = cb_malloc(4096);
    EXPECT_EQ(sz2, cb::ArenaMalloc::getPreciseAllocated(client));
    EXPECT_EQ(cb::ArenaMalloc::getPreciseAllocated(client),
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));

    // 3) Track deallocation
    cb::ArenaMalloc::switchToClient(client);
    cb_free(p);
    cb::ArenaMalloc::switchFromClient();
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));
    EXPECT_EQ(cb::ArenaMalloc::getPreciseAllocated(client),
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));
    cb_free(p2);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));
    EXPECT_EQ(cb::ArenaMalloc::getPreciseAllocated(client),
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));

    // 4) Track an allocation in a different domain
    auto allocAndCheck = [&client]() {
        auto p = cb_malloc(4096);
        EXPECT_EQ(4096, cb::ArenaMalloc::getPreciseAllocated(client));
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Primary));

        EXPECT_EQ(4096,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Secondary));
        cb_free(p);

        EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Primary));
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Secondary));
    };
    cb::ArenaMalloc::switchToClient(client);
    cb::ArenaMalloc::setDomain(cb::MemoryDomain::Secondary);
    allocAndCheck();

    // 5) Track an allocation in a different domain direct switchTo variant
    cb::ArenaMalloc::switchToClient(client, cb::MemoryDomain::Secondary);
    allocAndCheck();

    cb::ArenaMalloc::unregisterClient(client);
}

TEST_F(ArenaMalloc, DomainGuard) {
    auto client = cb::ArenaMalloc::registerClient();
    auto domain = cb::ArenaMalloc::switchToClient(client);
    EXPECT_EQ(cb::MemoryDomain::None, domain);

    void *p1, *p2;
    {
        cb::UseArenaMallocSecondaryDomain domain;
        p1 = cb_malloc(4096);
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Primary));
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getEstimatedAllocated(
                          client, cb::MemoryDomain::Primary));
        EXPECT_EQ(4096,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Secondary));
        EXPECT_EQ(4096,
                  cb::ArenaMalloc::getEstimatedAllocated(
                          client, cb::MemoryDomain::Secondary));
    }
    p2 = cb_malloc(8192);
    EXPECT_EQ(8192,
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));
    EXPECT_EQ(8192,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Primary));
    EXPECT_EQ(4096,
              cb::ArenaMalloc::getPreciseAllocated(
                      client, cb::MemoryDomain::Secondary));
    EXPECT_EQ(4096,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Secondary));

    {
        cb::UseArenaMallocSecondaryDomain domain;
        cb_free(p1);
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getPreciseAllocated(
                          client, cb::MemoryDomain::Secondary));
        EXPECT_EQ(0,
                  cb::ArenaMalloc::getEstimatedAllocated(
                          client, cb::MemoryDomain::Secondary));
        // Finally demonstrate that if the caller gets the domain wrong - we
        // cannot figure it out for them. P2 should be freed against Primary.
        // Same issue could happen if the wrong client was used
        cb_free(p2);
    }

    // Counters will hit 0 (we wouldn't report negative mem_used)
    EXPECT_EQ(0,
              cb::ArenaMalloc::getPreciseAllocated(
                      client, cb::MemoryDomain::Secondary));
    EXPECT_EQ(0,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Secondary));
    // Uh-oh
    EXPECT_EQ(8192,
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));
    EXPECT_EQ(8192,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Primary));

    domain = cb::ArenaMalloc::switchToClient(client,
                                             cb::MemoryDomain::Secondary);
    EXPECT_EQ(cb::MemoryDomain::Primary, domain);
    domain = cb::ArenaMalloc::switchToClient(client, cb::MemoryDomain::Primary);
    EXPECT_EQ(cb::MemoryDomain::Secondary, domain);

    cb::ArenaMalloc::unregisterClient(client);
}

#if defined(HAVE_JEMALLOC)
// Only when we have jemalloc do we use the thresholds to defer totalling memory
// usage.
TEST_F(ArenaMalloc, thresholds) {
#else
TEST_F(ArenaMalloc, DISABLED_thresholds) {
#endif
    auto client = cb::ArenaMalloc::registerClient();
    client.estimateUpdateThreshold = 1024;
    cb::ArenaMalloc::setAllocatedThreshold(client);
    cb::ArenaMalloc::switchToClient(client);
    auto p1 = cb_malloc(100);

    // p1 is smaller than threshold - so estimate is expected to be 0
    EXPECT_EQ(0, cb::ArenaMalloc::getEstimatedAllocated(client));
    // the domain as well reports 0
    EXPECT_EQ(0,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Primary));

    // calling precise will update the estimates (total and domains)
    auto p1val = cb::ArenaMalloc::getPreciseAllocated(client);
    EXPECT_NE(0, p1val);
    EXPECT_EQ(p1val, cb::ArenaMalloc::getEstimatedAllocated(client));
    EXPECT_EQ(p1val,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Primary));
    EXPECT_EQ(0,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Secondary));
    EXPECT_EQ(p1val,
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));

    // Next exceeding the threshold will force update the estimates irrespective
    // of a call to getPrecise
    auto p2 = cb_malloc(1025);
    auto p2val = cb::ArenaMalloc::getEstimatedAllocated(client);
    EXPECT_GT(p2val, p1val); // memory increased
    EXPECT_EQ(p2val,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Primary));
    EXPECT_EQ(0,
              cb::ArenaMalloc::getEstimatedAllocated(
                      client, cb::MemoryDomain::Secondary));
    EXPECT_EQ(p2val, cb::ArenaMalloc::getPreciseAllocated(client));
    EXPECT_EQ(p2val,
              cb::ArenaMalloc::getPreciseAllocated(client,
                                                   cb::MemoryDomain::Primary));

    cb_free(p1);
    cb_free(p2);
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

    bool profConfigured = false;
    sz = sizeof(profConfigured);
    EXPECT_EQ(0, je_mallctl("config.prof", &profConfigured, &sz, nullptr, 0));
    // Jemalloc should have been configured with profiling support on Linux;
    // on macOS it doesn't work but might have been configured depending
    // on cbDep version.
    if (folly::kIsLinux) {
        EXPECT_TRUE(profConfigured);
    }
    if (profConfigured) {
        bool profEnabled = false;
        sz = sizeof(profEnabled);
        EXPECT_EQ(0, je_mallctl("opt.prof", &profEnabled, &sz, nullptr, 0));
        EXPECT_TRUE(profEnabled);
    }
}
#endif

/**
 * Showcase that tcache slabs are allocated from the current arena and that
 * consecutive allocations after a switch to a different arena can be served
 * from the slab allocated from the former arena.
 *
 * In effect, allocating memory from the "wrong" arena.
 */
#if defined(HAVE_JEMALLOC)
TEST_F(ArenaMalloc, MB_55268) {
    // We need tcache to demonstrate the issue
    auto oldTCache = cb::ArenaMalloc::setTCacheEnabled(true);
    // Other tests might expect tcache to be disabled
    auto restoreTCache = folly::makeGuard(
            [oldTCache]() { cb::ArenaMalloc::setTCacheEnabled(oldTCache); });

    auto getMemoryStats = [](auto& client) -> std::pair<size_t, size_t> {
        std::unordered_map<std::string, size_t> stats;
        cb::ArenaMalloc::getStats(client, stats);
        return {stats["allocated"], stats["resident"]};
    };

    // Create two arenas with tcache=enabled
    auto first = cb::ArenaMalloc::registerClient(true);
    auto second = cb::ArenaMalloc::registerClient(true);

    auto [initialFirstAllocated, initialFirstResident] = getMemoryStats(first);
    auto [initialSecondAllocated, initialSecondResident] =
            getMemoryStats(second);
    // Nothing has been allocated
    ASSERT_EQ(0, initialFirstAllocated);
    ASSERT_EQ(0, initialSecondAllocated);
    // But arenas have some overhead
    ASSERT_NE(0, initialFirstResident);
    ASSERT_NE(0, initialSecondResident);

    const size_t testAllocationSize = 3000;
    // Exhaust any pre-assigned memory to the arena
    const size_t testAllocations = initialFirstResident / testAllocationSize;

    for (size_t i = 0; i < testAllocations; i++) {
        // Allocate some bytes. Tcache will allocate a slab from the "first"
        // arena.
        cb::ArenaMalloc::switchToClient(first);
        cb::ArenaMalloc::malloc(3000);

        // Switch to arena "second" and allocate some more bytes. This
        // allocation will be serviced using the slab allocated from the "first"
        // arenas slab, already in the tcache.
        cb::ArenaMalloc::switchToClient(second);
        cb::ArenaMalloc::malloc(3000);

        // Switch back to the first arena -- the malloc(3000) is the only
        // allocation from "second".
        cb::ArenaMalloc::switchToClient(first);
    }

    // Flush the tcache -- any slabs are returned to their arenas.
    je_mallctl("thread.tcache.flush", nullptr, nullptr, nullptr, 0);

    auto [firstAllocated, firstResident] = getMemoryStats(first);
    auto [secondAllocated, secondResident] = getMemoryStats(second);

    // With per-arena tcaches, we should expect to have allocated some
    // memory from both arenas.
    EXPECT_NE(0, firstAllocated);
    EXPECT_LT(initialFirstResident, firstResident);
    EXPECT_NE(0, secondAllocated);
    EXPECT_LT(initialSecondResident, secondResident);
}
#endif // defined(HAVE_JEMALLOC)
