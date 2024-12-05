/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/thread_local_monotonic_resource.h>

#include <platform/cb_arena_malloc.h>
#include <utility>

namespace cb {

MonotonicBufferResource::MonotonicBufferResource(size_t initialSize,
                                                 size_t maxSize)
    : maxSize(maxSize) {
    cb::NoArenaGuard guard;
    initialBuffer.resize(initialSize);
    resource = std::make_unique<detail::monotonic_buffer_resource>(
            initialBuffer.data(), initialBuffer.size());
}

MonotonicBufferResource::~MonotonicBufferResource() {
    cb::NoArenaGuard guard;
    resource.reset();
    std::exchange(initialBuffer, {});
}

void* MonotonicBufferResource::allocate(size_t bytes) {
    if (allocatedBytes + bytes > maxSize) {
        throw std::bad_alloc();
    }

    cb::NoArenaGuard guard;
    void* ptr = resource->allocate(bytes);
    if (!ptr) {
        throw std::bad_alloc();
    }

    allocatedBytes += bytes;
    ++allocationCount;

    maxAllocatedBytes = std::max(maxAllocatedBytes, allocatedBytes);
    maxAllocationCount = std::max(maxAllocationCount, allocationCount);

    return ptr;
}

void MonotonicBufferResource::deallocate(void* ptr, size_t size) {
    allocatedBytes -= size;
    --allocationCount;
    if (!allocatedBytes) {
        cb::NoArenaGuard guard;
        // Time to reset the memory resource.
        resource->release();
    }
}

size_t MonotonicBufferResource::getAllocatedBytes() const {
    return allocatedBytes;
}

size_t MonotonicBufferResource::getAllocationCount() const {
    return allocationCount;
}

size_t MonotonicBufferResource::getMaxAllocatedBytes() const {
    return maxAllocatedBytes;
}

size_t MonotonicBufferResource::getMaxAllocationCount() const {
    return maxAllocationCount;
}

} // namespace cb
