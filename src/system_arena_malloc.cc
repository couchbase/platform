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

#include <platform/system_arena_malloc.h>

#include "relaxed_atomic.h"

#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#if defined(HAVE_MALLOC_USABLE_SIZE)
#include <malloc.h>
#endif

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

            // Set arena to +1 so we can distinguish no arena (0) vs an arena
            return ArenaMallocClient{index + 1, index, false};
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
    // Set to arena 0, no client, all tracking is disabled
    switchToClient({0, false, 0}, false /*tcache unused here*/);
}

size_t SystemArenaMalloc::getPreciseAllocated(const ArenaMallocClient& client) {
    // Just read from client's index into the allocations array
    return allocated[client.index];
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

void SystemArenaMalloc::free(void* ptr) {
    removeAllocation(ptr);
    MEM_ALLOC(free)(ptr);
}

void SystemArenaMalloc::sized_free(void* ptr, size_t size) {
    SystemArenaMalloc::free(ptr);
}

size_t SystemArenaMalloc::malloc_usable_size(const void* ptr) {
#if defined(HAVE_JEMALLOC)
    return je_malloc_usable_size(const_cast<void*>(ptr));
#elif defined(HAVE_MALLOC_USABLE_SIZE)
    return ::malloc_usable_size(const_cast<void*>(ptr));
#endif
    throw std::runtime_error(
            "SystemArenaMalloc::malloc_usable_size cannot be called");
}

bool SystemArenaMalloc::setTCacheEnabled(bool value) {
    (void)value; // ignored
    return false;
}

void SystemArenaMalloc::addAllocation(void* ptr) {
    if (canTrackAllocations()) {
        auto client = currentClient;
        if (client.arena) {
            allocated[client.index].fetch_add(
                    SystemArenaMalloc::malloc_usable_size(ptr));
        }
    }
}

void SystemArenaMalloc::removeAllocation(void* ptr) {
    if (canTrackAllocations()) {
        auto client = currentClient;
        if (client.arena) {
            allocated[client.index].fetch_sub(
                    SystemArenaMalloc::malloc_usable_size(ptr));
        }
    }
}

folly::Synchronized<
        std::array<SystemArenaMalloc::Client, ArenaMallocMaxClients>>
        SystemArenaMalloc::clients;
std::array<NonNegativeCounter<size_t, ClampAtZeroUnderflowPolicy>,
           ArenaMallocMaxClients>
        SystemArenaMalloc::allocated;

} // namespace cb
