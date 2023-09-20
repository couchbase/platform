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
#include <platform/non_negative_counter.h>
#include <platform/sized_buffer.h>

#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace cb {
template <class t>
class RelaxedAtomic;

/**
 * SystemArenaMalloc implements the ArenaMalloc class providing memory
 * allocation std memory functions (malloc/free etc...).
 *
 * The ArenaMalloc class is really designed for utilising je_malloc, but the
 * SystemArenaMalloc class provides some of the functionality.
 *
 * If the system provides malloc_usable_size then the SystemArenaMalloc class
 * will account allocation activity to the currently enabled (switchToClient)
 * client.
 *
 * All of the thread-cache parts of the API have no affect.
 *
 * Note that one of the downsides of the SystemArenaMalloc class vs
 * JEArenaMalloc is that it is less robust if deallocations occur against the
 * wrong client (i.e. malloc on client x, free on client y). With JEArenaMalloc
 * the deallocation will find it's way to the correct arena/client, with
 * SystemArenaMalloc the deallocation will be accounted against the wrong client
 * allowing for mem_used to attempt to go negative.
 *
 */
class SystemArenaMalloc {
public:
    struct ClientAndDomain {
        ArenaMallocClient client;
        MemoryDomain domain{MemoryDomain::None};
    };

    static ArenaMallocClient registerClient(bool threadCache);
    static void unregisterClient(const ArenaMallocClient& client);
    static MemoryDomain switchToClient(const ArenaMallocClient& client,
                                       cb::MemoryDomain domain,
                                       bool tcache);
    static MemoryDomain setDomain(MemoryDomain domain);
    static MemoryDomain switchFromClient();
    static void setAllocatedThreshold(const ArenaMallocClient& client) {
        // Does nothing
    }
    static size_t getPreciseAllocated(const ArenaMallocClient& client);
    static size_t getEstimatedAllocated(const ArenaMallocClient& client);

    static size_t getPreciseAllocated(const ArenaMallocClient& client,
                                      MemoryDomain domain);
    static size_t getEstimatedAllocated(const ArenaMallocClient& client,
                                        MemoryDomain domain);

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

    static bool getProperty(const char* name, unsigned& value);
    static bool getProperty(const char* name, size_t& value);
    static int setProperty(const char* name, const void* newp, size_t newlen);

    static void releaseMemory();
    static void releaseMemory(const ArenaMallocClient& client);

    static bool getStats(const ArenaMallocClient& client,
                         std::unordered_map<std::string, size_t>& statsMap);
    static bool getGlobalStats(
            std::unordered_map<std::string, size_t>& statsMap);
    static std::string getDetailedStats();
    static FragmentationStats getFragmentationStats(
            const ArenaMallocClient& client);
    static FragmentationStats getGlobalFragmentationStats();

private:
    static void addAllocation(void* ptr);
    static void removeAllocation(void* ptr);

    struct Client {
        void reset() {
            used = false;
        }
        bool used = false;
    };

    static folly::Synchronized<std::array<Client, ArenaMallocMaxClients>>
            clients;

    /**
     * Track memory used in an atomic non-negative counter (one per
     * domain), which for now uses the clamp at zero policy. MB-33900
     * captures one major issue which prevents the use of the throw
     * policy.
     *
     * One additional element for the domain means we account for the
     * "untracked" memory (i.e. after switchFromClient)
     *
     * One additional element (ArenaMallocMaxClients + 1) exists allow global
     * allocations to also be accounted for; they reside in the last element
     * (NoClientIndex).
     */
    using DomainCounter = std::array<
            AtomicNonNegativeCounter<size_t, ClampAtZeroUnderflowPolicy>,
            size_t(MemoryDomain::Count) + 1>;

    static std::array<DomainCounter, ArenaMallocMaxClients + 1> allocated;
};
} // namespace cb
