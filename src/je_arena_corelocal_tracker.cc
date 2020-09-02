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

#include "relaxed_atomic.h"
#include <folly/lang/Aligned.h>
#include <platform/cb_arena_malloc_client.h>
#include <platform/corestore.h>
#include <platform/je_arena_corelocal_tracker.h>
#include <platform/non_negative_counter.h>

#include <jemalloc/jemalloc.h>

#include <algorithm>
#include <array>

namespace cb {

// For each client we store the following information.
//
// 1) A CoreStore to track 'per core' current allocation total
// 2) A core allocation threshold. A signed 64-bit counter for how much we
//    will allow 1) to accumulate before a) updating the estimate 3) and b)
//    clearing the core count.
// 3) Estimated memory - a signed 64-bit counter of how much the client has
//    allocated. This value is updated by a) a thread reaching the per thread
//    allocation threshold, or b) a call to getPreciseAllocated. This counter
//    is signed so we can safely handle the counter validly being negative (see
//    comments in getPreciseAllocated and getEstimatedAllocated).

static std::array<
        CoreStore<folly::cacheline_aligned<cb::RelaxedAtomic<int64_t>>>,
        ArenaMallocMaxClients>
        coreAllocated;

struct ClientData {
    cb::RelaxedAtomic<int64_t> coreThreshold;
    cb::RelaxedAtomic<int64_t> clientEstimatedMemory;
};

// One per client and cacheline pad
static std::array<folly::cacheline_aligned<ClientData>, ArenaMallocMaxClients>
        clientData;

void JEArenaCoreLocalTracker::clientRegistered(
        const ArenaMallocClient& client) {
    clientData[client.index]->clientEstimatedMemory.store(0);
    for (auto& core : coreAllocated[client.index]) {
        core->store(0);
    }
    setAllocatedThreshold(client);
}

size_t JEArenaCoreLocalTracker::getPreciseAllocated(
        const ArenaMallocClient& client) {
    for (auto& core : coreAllocated[client.index]) {
        clientData[client.index]->clientEstimatedMemory.fetch_add(
                core.get()->exchange(0));
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

void JEArenaCoreLocalTracker::setAllocatedThreshold(
        const ArenaMallocClient& client) {
    clientData[client.index]->coreThreshold = client.estimateUpdateThreshold;
}

void maybeUpdateEstimatedTotalMemUsed(
        uint8_t index,
        folly::cacheline_aligned<cb::RelaxedAtomic<int64_t>>& core,
        int64_t value) {
    if (std::abs(value) > clientData[index]->coreThreshold) {
        clientData[index]->clientEstimatedMemory.fetch_add(core->exchange(0));
    }
}

void JEArenaCoreLocalTracker::memAllocated(uint8_t index, size_t size) {
    if (index != NoClientIndex) {
        size = je_nallocx(size, 0 /* flags aren't read in this call*/);
        auto& core = coreAllocated[index].get();
        auto newSize = core->fetch_add(size) + size;
        maybeUpdateEstimatedTotalMemUsed(index, core, newSize);
    }
}

void JEArenaCoreLocalTracker::memDeallocated(uint8_t index, void* ptr) {
    if (index != NoClientIndex) {
        auto size = je_sallocx(ptr, 0 /* flags aren't read in this call*/);
        auto& core = coreAllocated[index].get();
        auto newSize = core->fetch_sub(size) - size;
        maybeUpdateEstimatedTotalMemUsed(index, core, newSize);
    }
}

void JEArenaCoreLocalTracker::memDeallocated(uint8_t index, size_t size) {
    if (index != NoClientIndex) {
        size = je_nallocx(size, 0 /* flags aren't read in this call*/);
        auto& core = coreAllocated[index].get();
        auto newSize = core->fetch_sub(size) - size;
        maybeUpdateEstimatedTotalMemUsed(index, core, newSize);
    }
}

} // end namespace cb
