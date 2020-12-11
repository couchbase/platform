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

#include <platform/visibility.h>
#include <cstddef>
#include <cstdint>
#include <new>

namespace cb {

template <class T>
class RelaxedAtomic;

struct ArenaMallocClient;

/**
 * "plugin" Tracker for JEArenaMalloc utilising je_sallocx and CoreLocal
 * for tracking allocations.
 */
class PLATFORM_PUBLIC_API JEArenaCoreLocalTracker {
public:
    static void clientRegistered(const ArenaMallocClient& client);

    static void initialiseForNewThread(const ArenaMallocClient& client);

    /**
     * Update the threshold at which the per thread counter will synchronise
     * into the client total estimate.
     */
    static void setAllocatedThreshold(const ArenaMallocClient& client);

    /**
     * Return the precise memory used by the client, this triggers an
     * accumulation of each core local counter and will effect the returned
     * value from getEstimatedAllocated.
     */
    static size_t getPreciseAllocated(const ArenaMallocClient& client);

    /**
     * Return the estimated memory used by the client, this is an efficient read
     * of a single counter but can be ahead or behind the 'precise' value. The
     * lag should be bounded by the client's configured estimateUpdateThreshold
     * multiplied by the number of cores returned by cb::get_cpu_count
     */
    static size_t getEstimatedAllocated(const ArenaMallocClient& client);

    /**
     * Notify that memory was allocated to the client
     * @param index The index for the client who did the allocation
     * @param size The requested size of the allocation (i.e. size parameter to
     *        malloc)
     * @param alignment Alignment required for the allocation; if no additional
     *        alignment needed (over system default), specify 0.
     */
    static void memAllocated(uint8_t index,
                             size_t size,
                             std::align_val_t alignment = std::align_val_t{0});

    /**
     * Notify that memory was de-allocated by the client
     * @param index The index for the client who did the deallocation
     * @param ptr The allocation being deallocated
     */
    static void memDeallocated(uint8_t index, void* ptr);

    /**
     * Notify that memory was de-allocated by the client
     * @param index The index for the client who did the deallocation
     * @param size The size of the deallocation
     */
    static void memDeallocated(uint8_t index, size_t size);
};
} // end namespace cb
