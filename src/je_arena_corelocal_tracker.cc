/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "relaxed_atomic.h"
#include <folly/lang/Aligned.h>
#include <gsl/gsl-lite.hpp>
#include <platform/corestore.h>
#include <platform/je_arena_corelocal_tracker.h>
#include <platform/non_negative_counter.h>

#include <jemalloc/jemalloc.h>

#include <algorithm>
#include <array>

namespace cb {

// For each client we store the following information. Items 1 to 4 are accessed
// frequently - every alloc/dealloc that occurs will access these items.
//
// 1) Memory allocation/deallocation size is accounted into a relaxed atomic.
//    One per MemoryDomain.
using DomainAllocated =
        std::array<cb::RelaxedAtomic<int64_t>, size_t(MemoryDomain::Count)>;

// 2) The counter array is then cache-line aligned so that a set of counters
//    shouldn't straddle a cache-line. With two counters we don't.
using CPDomainAllocated = folly::cacheline_aligned<DomainAllocated>;

// 3) The counters are then stored in a CoreStore so that each executing thread
//    is accessing unique counters.
using CoreAllocated = CoreStore<CPDomainAllocated>;

// 4) An array of CoreStores gives each client their own CoreStore.
static std::array<CoreStore<CPDomainAllocated>, ArenaMallocMaxClients>
        coreAllocated;

// 5) ClientData defines counters used to store the total memory usage. At
//    certain events the per core counters are accumulated into these counters.
//    a) When the absolute value of a core counter exceeds coreThreshold
//    b) When any of the getPreciseAllocated functions are called
struct ClientData {
    cb::RelaxedAtomic<int64_t> coreThreshold;
    cb::RelaxedAtomic<int64_t> clientEstimatedMemory;
    std::array<cb::RelaxedAtomic<int64_t>, size_t(MemoryDomain::Count)>
            clientDomainEstimatedMemory;
};

// The client counters are stored cache-aligned, with one per client.
static std::array<folly::cacheline_aligned<ClientData>, ArenaMallocMaxClients>
        clientData;

void JEArenaCoreLocalTracker::clientRegistered(const ArenaMallocClient& client,
                                               bool arenaDebugChecksEnabled) {
    // CoreLocalTracker doesn't support debug checks.
    (void)arenaDebugChecksEnabled;
    // Clear all core/domain
    for (auto& core : coreAllocated[client.index]) {
        for (size_t domain = 0; domain < size_t(MemoryDomain::Count);
             domain++) {
            (*core.get())[domain].store(0);
        }
    }

    // clear estimates
    clientData[client.index]->clientEstimatedMemory = 0;
    clientData[client.index]->clientDomainEstimatedMemory.fill(0);

    setAllocatedThreshold(client);
}

size_t JEArenaCoreLocalTracker::getPreciseAllocated(
        const ArenaMallocClient& client) {
    for (auto& core : coreAllocated[client.index]) {
        for (size_t domain = 0; domain < size_t(MemoryDomain::Count);
             domain++) {
            auto value = (*core.get())[domain].exchange(0);
            clientData[client.index]->clientEstimatedMemory.fetch_add(value);
            clientData[client.index]
                    ->clientDomainEstimatedMemory[domain]
                    .fetch_add(value);
        }
    }

    // See the comment in getEstimatedAllocated regarding negative counts, even
    // in this case where we are summing up all core counters there is still
    // the possibility of seeing a negative value based. After we've observed
    // a core counter and summed it into the global count, it's not
    // impossible for an allocation to occur on that core and then be
    // deallocated on the next core, so our summation observes more
    // deallocations than allocations.
    return size_t(
            std::max(int64_t(0),
                     clientData[client.index]->clientEstimatedMemory.load()));
}

size_t JEArenaCoreLocalTracker::getEstimatedAllocated(
        const ArenaMallocClient& client) {
    // The client's memory counter could become negative.
    // For example if Core1 deallocates something it didn't allocate and the
    // deallocation triggers a sync of the core counter into the global
    // counter, this can lead to a negative value. In the negative case we
    // return zero so that we don't end up with a large unsigned value.
    return size_t(
            std::max(int64_t(0),
                     clientData[client.index]->clientEstimatedMemory.load()));
}

size_t JEArenaCoreLocalTracker::getPreciseAllocated(
        const ArenaMallocClient& client, MemoryDomain domain) {
    for (auto& core : coreAllocated[client.index]) {
        auto value = (*core.get())[size_t(domain)].exchange(0);
        clientData[client.index]->clientEstimatedMemory.fetch_add(value);
        clientData[client.index]
                ->clientDomainEstimatedMemory[size_t(domain)]
                .fetch_add(value);
    }
    return size_t(std::max(int64_t(0),
                           clientData[client.index]
                                   ->clientDomainEstimatedMemory[size_t(domain)]
                                   .load()));
}

size_t JEArenaCoreLocalTracker::getEstimatedAllocated(
        const ArenaMallocClient& client, MemoryDomain domain) {
    return size_t(std::max(int64_t(0),
                           clientData[client.index]
                                   ->clientDomainEstimatedMemory[size_t(domain)]
                                   .load()));
}

void JEArenaCoreLocalTracker::setAllocatedThreshold(
        const ArenaMallocClient& client) {
    clientData[client.index]->coreThreshold = client.estimateUpdateThreshold;
}

void maybeUpdateEstimatedTotalMemUsed(uint8_t index,
                                      MemoryDomain domain,
                                      cb::RelaxedAtomic<int64_t>& counter,
                                      int64_t value) {
    if (std::abs(value) > clientData[index]->coreThreshold) {
        auto value = counter.exchange(0);
        clientData[index]->clientEstimatedMemory.fetch_add(value);
        clientData[index]
                ->clientDomainEstimatedMemory[size_t(domain)]
                .fetch_add(value);
    }
}

void JEArenaCoreLocalTracker::memAllocated(uint8_t index,
                                           MemoryDomain domain,
                                           size_t size,
                                           std::align_val_t alignment) {
    if (index != NoClientIndex) {
        const int flags = (alignment > std::align_val_t{0})
                                  ? MALLOCX_ALIGN(alignment)
                                  : 0;
        size = je_nallocx(size, flags);
        auto& core = coreAllocated[index].get();
        auto& counter = (*core.get())[size_t(domain)];
        auto newSize = counter.fetch_add(size) + size;
        maybeUpdateEstimatedTotalMemUsed(index, domain, counter, newSize);
    }
}

void JEArenaCoreLocalTracker::memDeallocated(uint8_t index,
                                             MemoryDomain domain,
                                             void* ptr) {
    if (index != NoClientIndex) {
        auto size = je_sallocx(ptr, 0 /* flags aren't read in this call*/);
        auto& core = coreAllocated[index].get();
        auto& counter = (*core.get())[size_t(domain)];
        auto newSize = counter.fetch_sub(size) - size;
        maybeUpdateEstimatedTotalMemUsed(index, domain, counter, newSize);
    }
}

void JEArenaCoreLocalTracker::memDeallocated(uint8_t index,
                                             MemoryDomain domain,
                                             size_t size) {
    if (index != NoClientIndex) {
        size = je_nallocx(size, 0 /* flags aren't read in this call*/);
        auto& core = coreAllocated[index].get();
        auto& counter = (*core.get())[size_t(domain)];
        auto newSize = counter.fetch_sub(size) - size;
        maybeUpdateEstimatedTotalMemUsed(index, domain, counter, newSize);
    }
}

} // end namespace cb
