/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/je_arena_simple_tracker.h>
#include <platform/non_negative_counter.h>
#include "gsl/gsl-lite.hpp"

#include <jemalloc/jemalloc.h>

#include <array>
#include <numeric>

namespace cb {

/**
 * Track memory used in an atomic non-negative counter (one per
 * domain).
 *
 * Uses AtomicNonNegativeCounter configured with DynamicUnderflowPolicy,
 * to detect any underflow bugs with memory accounting (if mode set to
 * ThrowException).
 * DynamicUnderflowPolicy is used (as opposed to a strict
 * ThrowExceptionUnderflowPolicy) because not all tests are "underflow-clean",
 * so need to enable throwing only for tests which are expected to not
 * underflow.
 *
 * One additional element for the domain means we account for the
 * "untracked" memory (i.e. after switchFromClient)
 *
 * One additional element (ArenaMallocMaxClients + 1) i allocated
 * exists allow global allocations to also be accounted for; they reside
 * in the last element (NoClientIndex).
 */
using DomainCounter =
        std::array<AtomicNonNegativeCounter<size_t, DynamicUnderflowPolicy>,
                   size_t(MemoryDomain::Count) + 1>;

static std::array<DomainCounter, ArenaMallocMaxClients + 1> allocated;

void JEArenaSimpleTracker::clientRegistered(const ArenaMallocClient& client,
                                            bool arenaDebugChecksEnabled) {
    // If debug checks enabled, then configure allocated counters to throw
    // an exception on underflow.
    auto& clientData = allocated.at(client.index);
    for (auto& domain : clientData) {
        domain.mode = arenaDebugChecksEnabled
                              ? DomainCounter::value_type::Mode::ThrowException
                              : DomainCounter::value_type::Mode::ClampAtZero;
    }
    // Clear all domains
    std::fill(clientData.begin(), clientData.end(), 0);
}

void JEArenaSimpleTracker::clientUnregistered(const ArenaMallocClient& client) {
    // At unregister, outstanding allocated memory for each domain _should_
    // be zero.
    // TODO: This isn't currently true due to memory allocation issues. Enable
    // when it is.
}

size_t JEArenaSimpleTracker::getPreciseAllocated(
        const ArenaMallocClient& client) {
    const auto& clientData = allocated.at(client.index);
    return std::accumulate(clientData.begin(), clientData.end(), size_t(0));
}

size_t JEArenaSimpleTracker::getPreciseAllocated(
        const ArenaMallocClient& client, MemoryDomain domain) {
    auto& clientData = allocated.at(client.index);
    return clientData.at(uint8_t(domain)).load();
}

void JEArenaSimpleTracker::memAllocated(uint8_t index,
                                        MemoryDomain domain,
                                        size_t size,
                                        std::align_val_t alignment) {
    const int flags =
            (alignment > std::align_val_t{0}) ? MALLOCX_ALIGN(alignment) : 0;
    size = je_nallocx(size, flags);
    auto& clientData = allocated.at(index);
    auto& counter = clientData.at(uint8_t(domain));
    counter.fetch_add(size);
}

void JEArenaSimpleTracker::memDeallocated(uint8_t index,
                                          MemoryDomain domain,
                                          void* ptr) {
    auto size = je_sallocx(ptr, 0 /* flags aren't read in this call*/);
    auto& clientData = allocated.at(index);
    auto& counter = clientData.at(uint8_t(domain));
    counter.fetch_sub(size);
}

void JEArenaSimpleTracker::memDeallocated(uint8_t index,
                                          MemoryDomain domain,
                                          size_t size) {
    // Get "real" size of allocation in jemalloc, not necessarily the same as
    // what the application thinks it allocated (due to jemalloc size class
    // rounding up).
    size = je_nallocx(size, 0 /* flags aren't read in this call*/);
    auto& clientData = allocated.at(index);
    auto& counter = clientData.at(uint8_t(domain));
    counter.fetch_sub(size);
}

} // end namespace cb
