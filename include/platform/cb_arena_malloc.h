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
     * @param client The client to account to.
     */
    static void switchToClient(const ArenaMallocClient& client) {
        Impl::switchToClient(client);
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

#if defined(HAVE_MALLOC_USABLE_SIZE)
    /// @return the real size of the allocation
    static size_t malloc_usable_size(void* ptr) {
        return Impl::malloc_usable_size(ptr);
    }
#endif

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
     * @param value true if thread caching is enabled
     */
    static void setTCacheEnabled(bool value) {
        return Impl::setTCacheEnabled(value);
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
