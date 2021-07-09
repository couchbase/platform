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

#include <folly/Synchronized.h>
#include <platform/cb_arena_malloc_client.h>
#include <platform/je_arena_corelocal_tracker.h>
#include <platform/non_negative_counter.h>
#include <platform/sized_buffer.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace cb {

/**
 * JEArenaMalloc implements the ArenaMalloc class providing memory allocation
 * via je_malloc - https://github.com/jemalloc/jemalloc.
 *
 * registering for an arena gives the client a je_malloc arena to encapsulate
 * their allocation activity and providing mem_used functionality.
 *
 */
template <class trackingImpl>
class _JEArenaMalloc {
public:
    static ArenaMallocClient registerClient(bool threadCache);
    static void unregisterClient(const ArenaMallocClient& client);
    static void switchToClient(const ArenaMallocClient& client,
                               cb::MemoryDomain domain,
                               bool tcache);
    static MemoryDomain setDomain(MemoryDomain domain);
    static void switchFromClient();
    static void setAllocatedThreshold(const ArenaMallocClient& client) {
        trackingImpl::setAllocatedThreshold(client);
    }
    static size_t getPreciseAllocated(const ArenaMallocClient& client) {
        return trackingImpl::getPreciseAllocated(client);
    }
    static size_t getEstimatedAllocated(const ArenaMallocClient& client) {
        return trackingImpl::getEstimatedAllocated(client);
    }
    static size_t getPreciseAllocated(const ArenaMallocClient& client,
                                      MemoryDomain domain) {
        return trackingImpl::getPreciseAllocated(client, domain);
    }
    static size_t getEstimatedAllocated(const ArenaMallocClient& client,
                                        MemoryDomain domain) {
        return trackingImpl::getEstimatedAllocated(client, domain);
    }

    static void* malloc(size_t size);
    static void* calloc(size_t nmemb, size_t size);
    static void* realloc(void* ptr, size_t size);
    static void* aligned_alloc(size_t alignment, size_t size);
    static void free(void* ptr);
    static void aligned_free(void* ptr);
    static void sized_free(void* ptr, size_t size);
    static size_t malloc_usable_size(const void* ptr);
    static constexpr bool canTrackAllocations() {
        return true;
    }
    static bool setTCacheEnabled(bool value);

    static bool getProperty(const char* name, size_t& value);
    static int setProperty(const char* name, const void* newp, size_t newlen);

    static void releaseMemory();
    static void releaseMemory(const ArenaMallocClient& client);

    static bool getStats(const ArenaMallocClient& client,
                         std::unordered_map<std::string, size_t>& statsMap);
    static bool getGlobalStats(
            std::unordered_map<std::string, size_t>& statsMap);
    static void getDetailedStats(void (*callback)(void*, const char*),
                                 void* cbopaque);
    static FragmentationStats getFragmentationStats(
            const ArenaMallocClient& client);
    static FragmentationStats getGlobalFragmentationStats();

protected:
    static void clientRegistered(const ArenaMallocClient& client) {
        trackingImpl::clientRegistered(client);
    }

    /**
     * Called when memory is allocated
     *
     * @param index The index for the client who did the allocation
     * @param size The clients requested allocation size
     * @param alignment Alignment required for the allocation; if no additional
     *        alignment needed (over system default), specify 0.
     */
    static void memAllocated(uint8_t index,
                             MemoryDomain domain,
                             size_t size,
                             std::align_val_t alignment = std::align_val_t{0}) {
        trackingImpl::memAllocated(index, domain, size, alignment);
    }

    /**
     * Called when memory is deallocated
     *
     * @param index The index for the client who did the deallocation
     * @param ptr The allocation being deallocated
     */
    static void memDeallocated(uint8_t index, MemoryDomain domain, void* ptr) {
        trackingImpl::memDeallocated(index, domain, ptr);
    }

    /**
     * Called when memory is deallocated from a sized delete
     *
     * @param index The index for the client who did the deallocation
     * @param size The deallocation size
     */
    static void memDeallocated(uint8_t index,
                               MemoryDomain domain,
                               size_t size) {
        trackingImpl::memDeallocated(index, domain, size);
    }
};

using JEArenaMalloc = _JEArenaMalloc<JEArenaCoreLocalTracker>;

} // namespace cb
