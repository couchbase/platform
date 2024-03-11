/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>

#include <gtest/gtest.h>
#include <platform/thread_local_monotonic_resource.h>
#include <new>
#include <thread>

/// Check that we can allocate successfully and the counters get updated.
TEST(ThreadLocalMonotonicResource, Basic) {
    struct Tag {};
    using Resource = cb::ThreadLocalMonotonicResource<Tag,
                                                      10 * sizeof(int),
                                                      10 * sizeof(int)>;
    using Allocator = Resource::Allocator<int>;

    Allocator a;
    auto* ptr = a.allocate(5);

    EXPECT_EQ(5 * sizeof(int), a.getUnderlyingBuffer().getAllocatedBytes());
    EXPECT_EQ(5 * sizeof(int), a.getUnderlyingBuffer().getMaxAllocatedBytes());
    EXPECT_EQ(1, a.getUnderlyingBuffer().getAllocationCount());
    EXPECT_EQ(1, a.getUnderlyingBuffer().getMaxAllocationCount());
    a.deallocate(ptr, 5);
    EXPECT_EQ(0, a.getUnderlyingBuffer().getAllocatedBytes());
    EXPECT_EQ(5 * sizeof(int), a.getUnderlyingBuffer().getMaxAllocatedBytes());
    EXPECT_EQ(0, a.getUnderlyingBuffer().getAllocationCount());
    EXPECT_EQ(1, a.getUnderlyingBuffer().getMaxAllocationCount());
}

/// Check that we can make multiple allocations
TEST(ThreadLocalMonotonicResource, MultipleAllocs) {
    struct Tag {};
    using Resource = cb::ThreadLocalMonotonicResource<Tag,
                                                      10 * sizeof(int),
                                                      20 * sizeof(int)>;
    using Allocator = Resource::Allocator<int>;

    Allocator a;
    auto* ptr = a.allocate(15);
    auto* ptr2 = a.allocate(5);

    EXPECT_EQ(20 * sizeof(int), a.getUnderlyingBuffer().getAllocatedBytes());
    EXPECT_EQ(20 * sizeof(int), a.getUnderlyingBuffer().getMaxAllocatedBytes());
    EXPECT_EQ(2, a.getUnderlyingBuffer().getAllocationCount());
    EXPECT_EQ(2, a.getUnderlyingBuffer().getMaxAllocationCount());

    a.deallocate(ptr, 15);
    EXPECT_EQ(5 * sizeof(int), a.getUnderlyingBuffer().getAllocatedBytes());
    EXPECT_EQ(20 * sizeof(int), a.getUnderlyingBuffer().getMaxAllocatedBytes());
    EXPECT_EQ(1, a.getUnderlyingBuffer().getAllocationCount());
    EXPECT_EQ(2, a.getUnderlyingBuffer().getMaxAllocationCount());

    a.deallocate(ptr2, 5);
    EXPECT_EQ(0, a.getUnderlyingBuffer().getAllocatedBytes());
    EXPECT_EQ(20 * sizeof(int), a.getUnderlyingBuffer().getMaxAllocatedBytes());
    EXPECT_EQ(0, a.getUnderlyingBuffer().getAllocationCount());
    EXPECT_EQ(2, a.getUnderlyingBuffer().getMaxAllocationCount());
}

/// Check that there is an allocation limit
TEST(ThreadLocalMonotonicResource, Limit) {
    struct Tag {};
    using Resource = cb::ThreadLocalMonotonicResource<Tag,
                                                      10 * sizeof(int),
                                                      20 * sizeof(int)>;
    using Allocator = Resource::Allocator<int>;
    Allocator a;

    a.allocate(10);
    EXPECT_THROW(a.allocate(11), std::bad_alloc);
    EXPECT_NO_THROW(a.allocate(10));
}

/// Check that the underlying buffer is thread-local.
TEST(ThreadLocalMonotonicResource, ThreadLocal) {
    struct Tag {};
    using Resource = cb::ThreadLocalMonotonicResource<Tag, 0, 0>;
    using Allocator = Resource::Allocator<int>;

    Allocator a;
    auto& buffer = a.getUnderlyingBuffer();

    std::thread t(
            [&a, &buffer]() { EXPECT_NE(&buffer, &a.getUnderlyingBuffer()); });
    t.join();
}
