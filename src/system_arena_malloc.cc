/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/system_arena_malloc.h>

#include "relaxed_atomic.h"

#include <stdexcept>
#include <string>
#include <system_error>

#include <folly/portability/Malloc.h>

#include <gsl/gsl-lite.hpp>
// Why is jemalloc here?
// The system arena allocator is intended to be used when je_malloc is not
// present. However it is is possible with some manual changes to build the
// system allocator with jemalloc as the allocator (was useful in early
// development of the system arena code.
// Build this file when HAVE_JEMALLOC is true, and also change the
// cb_arena_alloc to pick the SystemArenaMalloc implementation.
#if defined(HAVE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#define MALLOC_PREFIX je_
#define CONCAT(A, B) A##B
#else
#include <cstdlib>
#define MALLOC_PREFIX std
#define CONCAT(A, B) A::B
#endif

#define CONCAT2(A, B) CONCAT(A, B)
#define MEM_ALLOC(name) CONCAT2(MALLOC_PREFIX, name)

namespace cb {

static thread_local ArenaMallocClient currentClient;

ArenaMallocClient SystemArenaMalloc::registerClient(bool threadCache) {
    (void)threadCache; // Has no affect on system arena
    auto lockedClients = clients.wlock();
    // In the SystemArenaAllocator the client is just given an index which is
    // the 'arena' into which allocations are tracked (provided they use
    // switchToClient to set the client)
    for (uint8_t index = 0; index < lockedClients->size(); index++) {
        auto& client = lockedClients->at(index);
        if (!client.used) {
            client.used = true;

            // arena and threadCache unused for SystemArenaMalloc.
            return ArenaMallocClient{/*used*/ 0, index, /*unused*/ false};
        }
    }
    throw std::runtime_error(
            "SystemArenaMalloc::registerClient no available slots");
}

void SystemArenaMalloc::unregisterClient(const ArenaMallocClient& client) {
    clients.wlock()->at(client.index).reset();
}

void SystemArenaMalloc::switchToClient(const ArenaMallocClient& client,
                                       bool tcache) {
    currentClient = client;
    (void)tcache; // no use in system allocator
}

void SystemArenaMalloc::switchFromClient() {
    // Set to index of NoClientIndex; arena unused.
    switchToClient({0, NoClientIndex, false}, false /*tcache unused here*/);
}

size_t SystemArenaMalloc::getPreciseAllocated(const ArenaMallocClient& client) {
    // Just read from client's index into the allocations array
    return allocated.at(client.index);
}

size_t SystemArenaMalloc::getEstimatedAllocated(
        const ArenaMallocClient& client) {
    return getPreciseAllocated(client);
}

void* SystemArenaMalloc::malloc(size_t size) {
    void* newAlloc = MEM_ALLOC(malloc)(size);
    addAllocation(newAlloc);
    return newAlloc;
}

void* SystemArenaMalloc::calloc(size_t nmemb, size_t size) {
    void* newAlloc = MEM_ALLOC(calloc)(nmemb, size);
    addAllocation(newAlloc);
    return newAlloc;
}

void* SystemArenaMalloc::realloc(void* ptr, size_t size) {
    removeAllocation(ptr);
    void* newAlloc = MEM_ALLOC(realloc)(ptr, size);
    addAllocation(newAlloc);
    return newAlloc;
}

void* SystemArenaMalloc::aligned_alloc(size_t alignment, size_t size) {
    void* newAlloc = nullptr;
#if defined(HAVE_ALIGNED_ALLOC)
    newAlloc = ::aligned_alloc(alignment, size);
#elif defined(WIN32)
    newAlloc = _aligned_malloc(size, alignment);
#elif defined(HAVE_POSIX_MEMALIGN)
    if (posix_memalign(&newAlloc, alignment, size)) {
        newAlloc = nullptr;
    }
#else
#error No underlying API for aligned memory available.
#endif
    addAllocation(newAlloc);
    return newAlloc;
}

void SystemArenaMalloc::free(void* ptr) {
    removeAllocation(ptr);
    MEM_ALLOC(free)(ptr);
}

void SystemArenaMalloc::aligned_free(void* ptr) {
    removeAllocation(ptr);
    // Apart from Win32, can use normal free().
#if defined(WIN32)
    _aligned_free(ptr);
#else
    MEM_ALLOC(free)(ptr);
#endif
}

void SystemArenaMalloc::sized_free(void* ptr, size_t size) {
    SystemArenaMalloc::free(ptr);
}

size_t SystemArenaMalloc::malloc_usable_size(const void* ptr) {
#if defined(HAVE_JEMALLOC)
    return je_malloc_usable_size(const_cast<void*>(ptr));
#else
    return ::malloc_usable_size(const_cast<void*>(ptr));
#endif
}

bool SystemArenaMalloc::setTCacheEnabled(bool value) {
    (void)value; // ignored
    return false;
}

bool SystemArenaMalloc::getProperty(const char* name, size_t& value) {
    return false;
}

int SystemArenaMalloc::setProperty(const char* name,
                                   const void* newp,
                                   size_t newlen) {
    return 0;
}

void SystemArenaMalloc::releaseMemory() {
}

void SystemArenaMalloc::releaseMemory(const ArenaMallocClient& client) {
    (void)client;
}

bool SystemArenaMalloc::getStats(
        const ArenaMallocClient& client,
        std::unordered_map<std::string, size_t>& statsMap) {
    statsMap["allocated"] = allocated.at(client.index);
    return true;
}

bool SystemArenaMalloc::getGlobalStats(
        std::unordered_map<std::string, size_t>& statsMap) {
    statsMap["allocated"] = allocated.at(NoClientIndex);
    return true;
}

void SystemArenaMalloc::getDetailedStats(void (*callback)(void*, const char*),
                                         void* cbopaque) {
}

cb::FragmentationStats SystemArenaMalloc::getFragmentationStats(
        const cb::ArenaMallocClient& client) {
    size_t alloc = allocated.at(client.index);
    return {alloc, alloc};
}

cb::FragmentationStats SystemArenaMalloc::getGlobalFragmentationStats() {
    size_t alloc = allocated.at(NoClientIndex);
    return {alloc, alloc};
}

void SystemArenaMalloc::addAllocation(void* ptr) {
    if (canTrackAllocations()) {
        auto client = currentClient;
        allocated.at(client.index)
                .fetch_add(SystemArenaMalloc::malloc_usable_size(ptr));
    }
}

void SystemArenaMalloc::removeAllocation(void* ptr) {
    if (canTrackAllocations()) {
        auto client = currentClient;
        allocated.at(client.index)
                .fetch_sub(SystemArenaMalloc::malloc_usable_size(ptr));
    }
}

folly::Synchronized<
        std::array<SystemArenaMalloc::Client, ArenaMallocMaxClients>>
        SystemArenaMalloc::clients;
std::array<NonNegativeCounter<size_t, ClampAtZeroUnderflowPolicy>,
           ArenaMallocMaxClients + 1>
        SystemArenaMalloc::allocated;

} // namespace cb

#undef MALLOC_PREFIX
#undef CONCAT
#undef CONCAT2
#undef MEM_ALLOC
