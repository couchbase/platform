/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#if __has_include(<memory_resource>)

#ifdef __APPLE__
#warning MacOS now supports <memory_resource> and we do not need Boost.Container
#endif

#include <memory_resource>
namespace cb::detail {
using monotonic_buffer_resource = std::pmr::monotonic_buffer_resource;
}
#else
/// Fallback for macOS, where this C++17 header is not available.
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
namespace cb::detail {
using monotonic_buffer_resource =
        boost::container::pmr::monotonic_buffer_resource;
}
#endif

#include <vector>

namespace cb {

/**
 * A thread-local memory resource.
 *
 * Provides an Allocator<T> type conforming to the Allocator concept. The
 * Allocator is a bump allocator which allocates from an existing memory buffer,
 * implemented using std::pmr::monotonic_buffer_resource. The memory buffer
 * starts at the specified initial size and is allowed to grow. The initial size
 * allocation is freed upon thread-exit.
 *
 * Deallocation is no-op with the exception that is decrements the counter for
 * currently allocated memory. When that count reached 0, all memory buffers
 * except the initial one are deallocated and the internal allocation pointer is
 * reset to the 0 offset.
 *
 * This allocator is intended for cases where some finite amount of work is
 * done. By using this allocator, all of the work can use a pre-allocated block
 * of memory, which is theoretically as fast as stackalloc.
 *
 * @tparam Tag is used to create a new template instantiation
 * @tparam InitialSize specifies the fixed initial buffer size used per-thread
 * @tparam MaxSize is used to impose a limit on the allocations
 */
template <typename Tag, size_t InitialSize, size_t MaxSize>
struct ThreadLocalMonotonicResource {
    struct Buffer {
    public:
        Buffer()
            : initialBuffer(InitialSize),
              resource(initialBuffer.data(), initialBuffer.size()),
              allocatedBytes(0),
              allocationCount(0),
              maxAllocatedBytes(0),
              maxAllocationCount(0) {
        }

        /**
         * Allocate memory.
         * @throw std::bad_alloc on failure
         */
        void* allocate(size_t bytes) {
            if (allocatedBytes + bytes > MaxSize) {
                throw std::bad_alloc();
            }

            void* ptr = resource.allocate(bytes);
            if (!ptr) {
                throw std::bad_alloc();
            }

            allocatedBytes += bytes;
            ++allocationCount;

            maxAllocatedBytes = std::max(maxAllocatedBytes, allocatedBytes);
            maxAllocationCount = std::max(maxAllocationCount, allocationCount);

            return ptr;
        }

        /**
         * Deallocate memory.
         */
        void deallocate(void* ptr, size_t size) {
            allocatedBytes -= size;
            --allocationCount;
            if (!allocatedBytes) {
                // Time to reset the memory resource.
                resource.release();
            }
        }

        /**
         * @return the current allocated memory
         */
        size_t getAllocatedBytes() const {
            return allocatedBytes;
        }

        /**
         * @return the current number of allocations
         */
        size_t getAllocationCount() const {
            return allocationCount;
        }

        /**
         * The maximum memory we ever allocated from this allocator.
         */
        size_t getMaxAllocatedBytes() const {
            return maxAllocatedBytes;
        }

        /**
         * The maximum number of allocations we ever made from this allocator.
         */
        size_t getMaxAllocationCount() const {
            return maxAllocationCount;
        }

    private:
        /// The initial buffer allocated. Gets deallocated on thread exit.
        std::vector<char> initialBuffer;
        /// A bump allocator.
        detail::monotonic_buffer_resource resource;
        /// Current allocated memory.
        size_t allocatedBytes;
        /// Current number of allocations.
        size_t allocationCount;
        /// The maximum memory we ever allocated from this allocator.
        size_t maxAllocatedBytes;
        /// The maximum number of allocations we ever made from this allocator.
        size_t maxAllocationCount;
    };

    /**
     * Provides a lazily-initialised thread-local Buffer.
     */
    static Buffer& getThreadBuffer() {
        thread_local Buffer buffer;
        return buffer;
    }

    /**
     * Type implementing the Allocator concept.
     * Allocates from the thread_local Buffer.
     */
    template <typename T>
    struct Allocator {
        using value_type = T;

        /**
         * Access the internal buffer (for debugging).
         */
        Buffer& getUnderlyingBuffer() const {
            return getThreadBuffer();
        }

        /**
         * Allocates memory for n elements of type T.
         */
        T* allocate(size_t n) const {
            auto& buf = getThreadBuffer();
            return static_cast<T*>(buf.allocate(sizeof(T) * n));
        }

        /**
         * Releases memory for n elements of type T.
         */
        void deallocate(T* ptr, size_t n) const {
            auto& buf = getThreadBuffer();
            return buf.deallocate(ptr, sizeof(T) * n);
        }

        /**
         * This type has no state, so all instances compare equal.
         */
        friend constexpr bool operator==(Allocator, Allocator) {
            return true;
        }
        friend constexpr bool operator!=(Allocator, Allocator) {
            return false;
        }
    };
};

} // namespace cb
