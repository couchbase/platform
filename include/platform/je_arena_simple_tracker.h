/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <platform/cb_arena_malloc_client.h>
#include <new>

namespace cb {

struct ArenaMallocClient;

/**
 * A memory tracker using je_sallocx to determine the real size of a given
 * allocation, then a per-arena counter tracking how much memory has been
 * allocated.
 * (Much) lower performance than JEArenaCoreLocalTracker, but useful for
 * debugging memory issues as it's statistics are always accurate (no per-core
 * caching of memory allocated / deallocated), and it can verify that memory
 * tracking counters are never negative (this is not possible to assert with
 * the core-local counters without merging them, which negates their performance
 * benefit).
 *
 * Q: How does this compare to just using SystemArenaMalloc?
 * A: SystemArenaMalloc also has simple per-arena counters, however it doesn't
 *    use jemalloc's arenas, and hence doesn't support things like per-client
 *    resident memory metrics. In other words this class is fully-featured
 *    for all KV-Engine use-cases, it just isn't as fast as
 *    JEArenaCoreLocalTracker.
 */
class JEArenaSimpleTracker {
public:
    static void clientRegistered(const ArenaMallocClient& client,
                                 bool arenaDebugChecksEnabled);

    static void clientUnregistered(const ArenaMallocClient& client);

    static void initialiseForNewThread(const ArenaMallocClient& client);

    /**
     * Update the threshold at which the per thread counter will synchronise
     * into the client total estimate.
     */
    static void setAllocatedThreshold(const ArenaMallocClient& client) {
        // no-op - tracking is always precise.
    }

    static bool isTrackingAlwaysPrecise() {
        return true;
    }

    /**
     * Return the precise memory used by the client.
     */
    static size_t getPreciseAllocated(const ArenaMallocClient& client);

    /**
     * Return the precise memory used by the client (memory usage is
     * not estimated with this tracker).
     */
    static size_t getEstimatedAllocated(const ArenaMallocClient& client) {
        return getPreciseAllocated(client);
    }

    /// as above, but lookup for the requested domain only
    static size_t getPreciseAllocated(const ArenaMallocClient& client,
                                      MemoryDomain domain);

    /// as above, but lookup for the requested domain only
    static size_t getEstimatedAllocated(const ArenaMallocClient& client,
                                        MemoryDomain domain) {
        return getPreciseAllocated(client, domain);
    }

    /**
     * Notify that memory was allocated to the client
     * @param index The index for the client who did the allocation
     * @param size The requested size of the allocation (i.e. size parameter to
     *        malloc)
     * @param alignment Alignment required for the allocation; if no additional
     *        alignment needed (over system default), specify 0.
     */
    static void memAllocated(uint8_t index,
                             MemoryDomain domain,
                             size_t size,
                             std::align_val_t alignment = std::align_val_t{0});

    /**
     * Notify that memory was de-allocated by the client
     * @param index The index for the client who did the deallocation
     * @param ptr The allocation being deallocated
     */
    static void memDeallocated(uint8_t index, MemoryDomain domain, void* ptr);

    /**
     * Notify that memory was de-allocated by the client
     * @param index The index for the client who did the deallocation
     * @param size The size of the deallocation
     */
    static void memDeallocated(uint8_t index, MemoryDomain domain, size_t size);
};

} // namespace cb
