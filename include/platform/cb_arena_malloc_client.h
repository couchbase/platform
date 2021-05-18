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

#include "relaxed_atomic.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cb {

/**
 * The maximum number of concurrently registered clients is set to 100.
 * jemalloc internally defines (in jemalloc_internal_types.h) the maximum arenas
 * to be (1<<12)-1, 32,767, however some early testing where we didn't recycle
 * arenas hinted that many arenas being allocated wasn't very performant. So we
 * will set this as 100, which is now the hard limit on concurrent buckets, but
 * way higher than the supported limit. The code also uses some non-dynamic data
 * which is sized using this value, raising this value will grow that data.
 *
 * Note that KV-engine will use this value to define the hard bucket limit.
 */
const int ArenaMallocMaxClients = 100;

/// Define a special value to denote that no client is selected
const uint8_t NoClientIndex = ArenaMallocMaxClients;

/**
 * The cb::ArenaMallocClient is an object that any client of the cb::ArenaMalloc
 * class must keep for use with cb::ArenaMalloc class.
 *
 * The client will receive a cb::ArenaMallocClient object when they call
 * cb::ArenaMalloc::registerClient and it must be kept until they call
 * cb::ArenaMalloc::unregister.
 */
struct ArenaMallocClient {
    ArenaMallocClient() {
    }

    ArenaMallocClient(int arena, uint8_t index, bool threadCache)
        : arena(arena), index(index), threadCache(threadCache) {
    }

    /**
     * Memory usage may be manually tracked by the library, the tracking use
     * per core counters and a core threshold for when the arena total is
     * calculated.
     *
     * This method allows that per-core threshold to be set.
     *
     * The threshold is calculated as:
     *    percentof(maxDataSize, percentage) / get_num_cpus
     *
     * @param maxDataSize the client (bucket's) maximum permitted memory usage
     * @param percentage the percentage (0.0 to 100.0, caller should validate)
     */
    void setEstimateUpdateThreshold(size_t maxDataSize, float percentage);

    /// How many bytes a core can alloc or dealloc before the arena's
    /// estimated memory is update.
    cb::RelaxedAtomic<uint32_t> estimateUpdateThreshold{100 * 1024};
    uint16_t arena{0}; // uniquely identifies the arena assigned to the client
    uint8_t index{NoClientIndex}; // uniquely identifies the registered client
    bool threadCache{true}; // should thread caching be used
};

} // namespace cb
