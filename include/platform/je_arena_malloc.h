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
    static void switchToClient(const ArenaMallocClient& client, bool tcache);
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
    static std::pair<size_t, size_t> getFragmentationStats(
            const ArenaMallocClient& client);
    static std::pair<size_t, size_t> getGlobalFragmentationStats();

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
                             size_t size,
                             std::align_val_t alignment = std::align_val_t{0}) {
        trackingImpl::memAllocated(index, size, alignment);
    }

    /**
     * Called when memory is deallocated
     *
     * @param index The index for the client who did the deallocation
     * @param ptr The allocation being deallocated
     */
    static void memDeallocated(uint8_t index, void* ptr) {
        trackingImpl::memDeallocated(index, ptr);
    }

    /**
     * Called when memory is deallocated from a sized delete
     *
     * @param index The index for the client who did the deallocation
     * @param size The deallocation size
     */
    static void memDeallocated(uint8_t index, size_t size) {
        trackingImpl::memDeallocated(index, size);
    }
};

using JEArenaMalloc = _JEArenaMalloc<JEArenaCoreLocalTracker>;

} // namespace cb
