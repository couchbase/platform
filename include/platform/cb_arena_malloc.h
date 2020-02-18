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

/**
 * ArenaMalloc provides a mechanism for a client to have all memory allocations
 * 'tracked' by allowing the client to first register for an 'arena' and then to
 * switch that arena on/off, effectively bracketing blocks of code so that all
 * allocation activity within that bracket occurs against the arena. If an
 * arena is not enabled (i.e. switchFromClient called) allocation activity
 * occurs against a default arena, in JEMalloc that is arena 0.
 *
 * Usage:
 * 1) client calls cb::ArenaMalloc::registerClient and saves the returned
 *    object.
 * 2) When the client wishes to have allocations tracked, they should switch
 *    to their arena. cb::ArenaMalloc::switchToClient/switchFromClient
 * 3) The client can query how much memory they have allocated using their arena
 *    and cb::ArenaMalloc::getAllocated
 *
 * Example:
 *
 *  auto client = cb::ArenaMalloc::registerClient();
 *  cb::ArenaMalloc::switchToClient(client);
 *  { .. do lots of stuff .. }
 *  cb::ArenaMalloc::switchFromClient(); // no more tracking
 *  auto mem_used = cb::ArenaMalloc::getAllocated(client);
 *
 * Note that the switchToClient/switchFromClient only affects the current
 * thread.
 */
#pragma once

#include <platform/cb_arena_malloc_client.h>
#include <memory>
#include <unordered_map>

#if defined(HAVE_JEMALLOC)
#include "je_arena_malloc.h"
#define ARENA_ALLOC JEArenaMalloc
#else
#include "system_arena_malloc.h"
#define ARENA_ALLOC SystemArenaMalloc
#endif

namespace cb {

template <class t>
class RelaxedAtomic;

template <class Impl>
class PLATFORM_PUBLIC_API _ArenaMalloc {
public:
    /**
     * Register a new client for allocation tracking
     * @param threadCache should this arena use a thread cache
     * @return An ArenaMallocClient object for use in subsequent calls
     */
    static ArenaMallocClient registerClient(bool threadCache = true) {
        return Impl::registerClient(threadCache);
    }

    /**
     * Unregister the client, freeing the arena for use by a new client.
     * @param client The client to unregister.
     */
    static void unregisterClient(const ArenaMallocClient& client) {
        Impl::unregisterClient(client);
    }

    /**
     * Switch to the given client, all subsequent memory allocations made by
     * the current thread will be accounted to the client.
     *
     * Note on thread cache (tcache). It is permitted to have clients with their
     * own tcache on/off, which is acheived by changing the value in the client
     * object, doing so is global to the client on all threads that execute the
     * client. Additionally we need to be able to switch tcache off for a single
     * thread, that is what the optional tcache parameter achieves.
     *
     * @param client The client to account to.
     * @param tcache The caller can switch to the client and turn off tcache
     *               for the current thread only.
     */
    static void switchToClient(const ArenaMallocClient& client,
                               bool tcache = true) {
        Impl::switchToClient(client, tcache);
    }

    /**
     * Set allocation threshold for the client, the compile time selected
     * tracker will use this value for updating the clients memory used.
     *
     * @param client The client object which stores the new threshold.
     */
    static void setAllocatedThreshold(const ArenaMallocClient& client) {
        Impl::setAllocatedThreshold(client);
    }

    /**
     * Switch away from the client, disabling any memory tracking.
     */
    static void switchFromClient() {
        Impl::switchFromClient();
    }

    /**
     * @param client The client to query
     * @return how many bytes are allocated to the client
     */
    static size_t getPreciseAllocated(const ArenaMallocClient& client) {
        return Impl::getPreciseAllocated(client);
    }

    /**
     * @param client The client to query
     * @return how many bytes are allocated to the client
     */
    static size_t getEstimatedAllocated(const ArenaMallocClient& client) {
        return Impl::getEstimatedAllocated(client);
    }

    /**
     * mimics std::malloc and tracks the allocation against the current client
     */
    static void* malloc(size_t size) {
        return Impl::malloc(size);
    }

    /**
     * mimics std::calloc and tracks the allocation against the current client
     */
    static void* calloc(size_t nmemb, size_t size) {
        return Impl::calloc(nmemb, size);
    }

    /**
     * mimics std::realloc and tracks the allocation against the current client
     */
    static void* realloc(void* ptr, size_t size) {
        return Impl::realloc(ptr, size);
    }

    /**
     * mimics std::free and tracks the deallocation against the current client
     */
    static void free(void* ptr) {
        Impl::free(ptr);
    }

    /**
     * sized_free  is an extension of free with a size parameter to allow the
     * caller to pass in the allocation size as an optimization.
     */
    static void sized_free(void* ptr, size_t size) {
        Impl::sized_free(ptr, size);
    }

    /**
     * @throws runtime_error if there is no malloc_usable_size to call
     * @return the real size of the allocation (ptr argument)
     */
    static size_t malloc_usable_size(const void* ptr) {
        return Impl::malloc_usable_size(ptr);
    }

    /**
     * @return true if the ArenaMalloc can correctly track allocations
     */
    static constexpr bool canTrackAllocations() {
        return Impl::canTrackAllocations();
    }

    /**
     * Set if thread caching is enabled, this overrides the registerClient
     * setting, i.e. set to false and no one should ever see use of a tcache.
     * Only JEMalloc implementation currently handles this, other
     * implementations ignore the call.
     *
     * @param value true if thread caching should be enabled, false to disable
     * @return the old value of tcache
     */
    static bool setTCacheEnabled(bool value) {
        return Impl::setTCacheEnabled(value);
    }

    /**
     * Get the value of an allocator size_t property.
     *
     * This is a thin wrapper for invoking je_mallctl or do nothing on system
     * allocator builds.
     *
     * @param name The name of the parameter to get (maps to mallctl name)
     * @param[out] value size_t to write to if successful (maps to mallctl oldp)
     * @return true if the call was successful
     */
    static bool getProperty(const char* name, size_t& value) {
        return Impl::getProperty(name, value);
    }

    /**
     * Set the value of an allocator property.
     *
     * This is a thin wrapper for invoking je_mallctl or do nothing on system
     * allocator builds.
     *
     * @param name The name of the parameter to get (maps to mallctl name)
     * @param newp new value to write to if successful (maps to mallctl newp)
     * @param newlen length of the newp data
     * @return the value returned by mallctl, consult jemalloc for info
     */
    static int setProperty(const char* name, const void* newp, size_t newlen) {
        return Impl::setProperty(name, newp, newlen);
    }

    /**
     * Request that unused memory is released from the process for all arenas
     */
    static void releaseMemory() {
        Impl::releaseMemory();
    }

    /**
     * Request that unused memory is released from the process for the clients
     * arena
     */
    static void releaseMemory(const ArenaMallocClient& client) {
        Impl::releaseMemory(client);
    }

    /**
     * Get a map of allocation stats for the given client.
     * @param client The client for which stats are needed
     * @param[out] statsMap a reference to a map to write to.
     * @return true if some stats are missing
     */
    static bool getStats(const ArenaMallocClient& client,
                         std::unordered_map<std::string, size_t>& statsMap) {
        return Impl::getStats(client, statsMap);
    }

    /**
     * Get a map of allocation stats for the arena used for allocations made
     * after calling switchFromClient
     * @param[out] statsMap a reference to a map to write to.
     */
    static bool getGlobalStats(
            std::unordered_map<std::string, size_t>& statsMap) {
        return Impl::getGlobalStats(statsMap);
    }

    /**
     * Return a detailed, human readable allocator statistic blob
     * @param buffer to write to
     */
    static void getDetailedStats(void (*callback)(void*, const char*),
                                 void* cbopaque) {
        return Impl::getDetailedStats(callback, cbopaque);
    }

    /**
     * Returns two values which can be used for calculating fragmentation:
     *  1) total allocated
     *  2) total resident
     *
     * 100% utilisation or 0% fragmentation would be when 1 and 2 are equal.
     *
     * @param client The client to get stats for
     * @return a pair, first is the total allocated and second is the total
     *         resident.
     */
    static std::pair<size_t, size_t> getFragmentationStats(
            const ArenaMallocClient& client) {
        return Impl::getFragmentationStats(client);
    }

    /**
     * Returns two values which can be used for calculating fragmentation:
     *  1) total allocated
     *  2) total resident
     *
     * 100% utilisation or 0% fragmentation would be when 1 and 2 are equal.
     *
     * This method returns the fragmentation stats for the arena holding non
     * bucket allocations.
     *
     * @return a pair, first is the total allocated and second is the total
     *         resident.
     */
    static std::pair<size_t, size_t> getGlobalFragmentationStats() {
        return Impl::getGlobalFragmentationStats();
    }
};

using ArenaMalloc = _ArenaMalloc<ARENA_ALLOC>;

/**
 *  ArenaMallocGuard will switch away from the client on destruction
 */
struct PLATFORM_PUBLIC_API ArenaMallocGuard {
    /// Will call switchToClient(client)
    ArenaMallocGuard(const ArenaMallocClient& client);

    /// Will call switchFromClient
    ~ArenaMallocGuard();
};

} // namespace cb
