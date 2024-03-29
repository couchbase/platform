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

#include <platform/cb_arena_malloc_client.h>

#include <cstddef>
#include <cstdint>
#include <new>

namespace cb {

template <class T>
class RelaxedAtomic;

struct ArenaMallocClient;

/**
 * "plugin" Tracker for JEArenaMalloc utilising je_sallocx and CoreLocal
 * for tracking allocations.
 */
class JEArenaCoreLocalTracker {
public:
    static void clientRegistered(const ArenaMallocClient& client,
                                 bool arenaDebugChecksEnabled);

    static void initialiseForNewThread(const ArenaMallocClient& client);

    /**
     * Update the threshold at which the per thread counter will synchronise
     * into the client total estimate.
     */
    static void setAllocatedThreshold(const ArenaMallocClient& client);

    static bool isTrackingAlwaysPrecise() {
        return false;
    }

    /**
     * Return the precise memory used by the client, this triggers an
     * accumulation of each core local counter and will effect the returned
     * value from getEstimatedAllocated.
     */
    static size_t getPreciseAllocated(const ArenaMallocClient& client);

    /**
     * Return the estimated memory used by the client, this is an efficient read
     * of a single counter but can be ahead or behind the 'precise' value. The
     * lag should be bounded by the client's configured estimateUpdateThreshold
     * multiplied by the number of cores returned by cb::get_cpu_count
     */
    static size_t getEstimatedAllocated(const ArenaMallocClient& client);

    /// as above, but lookup for the requested domain only
    static size_t getPreciseAllocated(const ArenaMallocClient& client,
                                      MemoryDomain domain);

    /// as above, but lookup for the requested domain only
    static size_t getEstimatedAllocated(const ArenaMallocClient& client,
                                        MemoryDomain domain);

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
} // end namespace cb
