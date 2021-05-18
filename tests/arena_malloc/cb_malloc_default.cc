/*
 *     Copyright 2020-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/cb_malloc.h>

#if defined(HAVE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#endif

#include <folly/portability/GTest.h>

// This program is not linked with the arenas library, so we expect cb_malloc
// to return false for the is_using_arenas function
TEST(CbMallocDefault, cb_malloc_is_not_using_arenas) {
    EXPECT_FALSE(cb_malloc_is_using_arenas());
}

#if defined(HAVE_JEMALLOC)
// This program was built with je_malloc available, so we expect that the
// default cb_malloc will call down to jemalloc
TEST(CbMallocDefault, cb_malloc_is_jemalloc) {
    // Grab the current allocated/deallocated values for this thread
    uint64_t allocated, deallocated;
    size_t len = sizeof(allocated);
    je_mallctl("thread.allocated", &allocated, &len, nullptr, 0);
    len = sizeof(deallocated);
    je_mallctl("thread.deallocated", &deallocated, &len, nullptr, 0);

    // do an allocation and check je_malloc allocated increases
    auto* p = cb_malloc(512);

    uint64_t allocated_1;
    len = sizeof(allocated_1);
    je_mallctl("thread.allocated", &allocated_1, &len, nullptr, 0);

    EXPECT_EQ(allocated_1, allocated + 512);

    // do a deallocation and check je_malloc deallocated increases
    cb_free(p);

    uint64_t deallocated_1;
    len = sizeof(deallocated_1);
    je_mallctl("thread.deallocated", &deallocated_1, &len, nullptr, 0);
    EXPECT_EQ(deallocated_1, deallocated + 512);
}
#endif