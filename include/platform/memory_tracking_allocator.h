/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#include <platform/non_negative_counter.h>

#include <memory>

/**
 * Class provides a generic memory allocator that allows memory usage to
 * be tracked.
 *
 * The allocator boilerplate was taken from the following web-site:
 * https://howardhinnant.github.io/allocator_boilerplate.html
 *
 * Example use:
 *
 * typedef std::deque<int, MemoryTrackingAllocator<int>> Deque;
 *
 *  MemoryTrackingAllocator<int> allocator;
 *  Deque theDeque(allocator);
 *
 *  To return the bytes allocated use:
 *
 *  theDeque.get_allocator().getBytesAllocated();
 *
 *  See /tests/memory_tracking_test/memory_tracking_allocator_test.cc for
 *  full code.
 *
 *  Note: A shared counter is required because it is possible to end up with
 *  multiple allocators, which need to allocate different object types.
 *  For example std::deque allocates space for the metadata and space for
 *  the items using separate allocators.  Therefore bytesAllocated needs to
 *  be a shared_ptr so that the bytes allocated by each allocator are
 *  combined together into a single value.
 *
 *  Also it is important to NOT use the default allocator of the container
 *  e.g. Deque theDeque().  Otherwise the allocator accounting will not be
 *  correct if the container rebinds, as is the case when using a std::deque
 *  container with libc++.
 *
 */

template <class T>
class MemoryTrackingAllocator {
public:
    using value_type = T;

    MemoryTrackingAllocator() noexcept
        : bytesAllocated(std::make_shared<cb::NonNegativeCounter<size_t>>(0)) {
    }

    template <class U>
    explicit MemoryTrackingAllocator(
            MemoryTrackingAllocator<U> const& other) noexcept
        /**
         * Used during a rebind and therefore need to copy over the
         * byteAllocated shared pointer.
         */
        : bytesAllocated(other.getUnderlyingCounter()) {
    }

    MemoryTrackingAllocator(const MemoryTrackingAllocator& other) noexcept =
            default;

    MemoryTrackingAllocator(MemoryTrackingAllocator&& other) noexcept
        // The move ctor can be invoked when the underlying container is
        // moved; however the old (moved-from) container while _logically_
        // empty could still _physically_ own allocations (e.g. a sentinal node
        // in a std::list). As such, we need to ensure the moved-from
        // container's allocator (i.e. other) can still perform deallocations,
        // hence bytesAllocated should only be copied, not moved-from.
        : bytesAllocated(other.getUnderlyingCounter()) {
    }

    MemoryTrackingAllocator& operator=(
            const MemoryTrackingAllocator& other) noexcept = default;

    MemoryTrackingAllocator& operator=(
            MemoryTrackingAllocator&& other) noexcept {
        // Need to copy bytesAllocated even though this is the move-assignment
        // operator - see comment in move ctor for rationale.
        bytesAllocated = other.getUnderlyingCounter();
        return *this;
    }

    value_type* allocate(std::size_t n) {
        *bytesAllocated += n * sizeof(T);
        return baseAllocator.allocate(n);
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        *bytesAllocated -= n * sizeof(T);
        baseAllocator.deallocate(p, n);
    }

    MemoryTrackingAllocator select_on_container_copy_construction() const {
        /**
         * We call the constructor to ensure that on a copy the new allocator
         * has its own byteAllocated counter instance.
         */
        return MemoryTrackingAllocator();
    }

    // @return the value of the bytesAllocated counter
    size_t getBytesAllocated() const {
        return *bytesAllocated;
    }

private:
    // @return a shared_ptr to the bytesAllocated counter
    auto getUnderlyingCounter() const {
        return bytesAllocated;
    }

    template <class U>
    friend class MemoryTrackingAllocator;

    /// The underlying "real" allocator which we actually use for
    /// allocating / deallocating memory.
    std::allocator<T> baseAllocator;

    std::shared_ptr<cb::NonNegativeCounter<size_t>> bytesAllocated;
};

template <class T, class U>
bool operator==(MemoryTrackingAllocator<T> const& a,
                MemoryTrackingAllocator<U> const& b) noexcept {
    return a.getBytesAllocated() == b.getBytesAllocated();
}

template <class T, class U>
bool operator!=(MemoryTrackingAllocator<T> const& a,
                MemoryTrackingAllocator<U> const& b) noexcept {
    return !(a == b);
}
