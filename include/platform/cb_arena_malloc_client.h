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

#include <fmt/ostream.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
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

enum class MemoryDomain : uint8_t { Primary, Secondary, Count, None = Count };

std::ostream& operator<<(std::ostream& os, const MemoryDomain& md);

// Map from MemoryDomain to the Arena to use for that domain. In production
// (NDEBUG) builds we use JEArenaCoreLocalTracker which always uses the same
// arena for all domains owned by a given client (to minimise arena usage),
// in Debug builds we use JEArenaSimpleTracker, which (if enabled at runtime via
// env var) can assign one arena per domain to allow us to check that memory
// is allocated / freed consistently against the correct domain.
using DomainToArena = std::array<uint16_t, size_t(MemoryDomain::Count)>;

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

    ArenaMallocClient(DomainToArena arenas, uint8_t index, bool threadCache)
        : arenas(arenas), index(index), threadCache(threadCache) {
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

    // Map each domain to arena to use for that domain.
    // The same arena may be used for multiple domains (production), or
    // one arena per domain (debug) depending on the build setting.
    DomainToArena arenas{0};
    uint8_t index{NoClientIndex}; // uniquely identifies the registered client
    bool threadCache{true}; // should thread caching be used
};

class FragmentationStats {
public:
    FragmentationStats(size_t allocatedBytes, size_t residentBytes)
        : allocatedBytes(allocatedBytes), residentBytes(residentBytes) {
    }

    size_t getAllocatedBytes() const {
        return allocatedBytes;
    }

    size_t getResidentBytes() const {
        return residentBytes;
    }

    /**
     * @return the fragmentation as a ratio, 0.0 to 1.0. A value of 1.0 means
     *             no resident bytes are allocated, 0.0 all bytes are allocated.
     */
    double getFragmentationRatio() const {
        return (double(residentBytes - allocatedBytes) / residentBytes);
    }

    /**
     * @return the fragmentation as a size, how many bytes resident but not
     * allocated.
     */
    size_t getFragmentationSize() const {
        return residentBytes - allocatedBytes;
    }

private:
    size_t allocatedBytes{0};
    size_t residentBytes{0};
};

std::ostream& operator<<(std::ostream&, const FragmentationStats&);

} // namespace cb

template <>
struct fmt::formatter<cb::MemoryDomain> : ostream_formatter {};
template <>
struct fmt::formatter<cb::FragmentationStats> : ostream_formatter {};
