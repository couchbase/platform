/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/cb_arena_malloc.h>
#include <platform/cb_malloc.h>

#include <cstring>
#include <new>
#include <string>
#include <thread>

// Test pointer in global scope to prevent compiler optimizing malloc/free away
// via DCE.
void* p;

class MemoryTrackerTest : public ::testing::Test {
public:
    MemoryTrackerTest() {
        client = cb::ArenaMalloc::registerClient();
    }
    ~MemoryTrackerTest() override {
        cb::ArenaMalloc::unregisterClient(client);
    }

    // function executed by our accounting test thread.
    void AccountingTestThread();
    std::thread thread;
    cb::ArenaMallocClient client;
};

void MemoryTrackerTest::AccountingTestThread() {
    cb::ArenaMallocGuard guard(client);

    // Test new & delete //////////////////////////////////////////////////
    //
    // Covers all variations of new & delete as of C++17
    // (https://en.cppreference.com/w/cpp/memory/new/operator_new &
    //  https://en.cppreference.com/w/cpp/memory/new/operator_delete)
    //
    // Note that the compiler will generally automatically use the "correct"
    // deallocation function for the particular new variant.
    // For example, allocating via new a type which requires extended alignment
    // will invoke 'operator new( std::size_t count, std::align_val_t al )',
    // and then calling 'delete' on it will invoke the aligned deletion
    // function.
    // However, to make it totally explicit exactly which new / delete
    // variant is being tested, we manually call the overload we are concerned
    // with.
    // (new X) or (del Y) annotations refer to the overload numbers from
    // the cppreference.com links above.
    // replaceable allocation functions:
    // void* operator new  ( std::size_t count );                       (new 1)
    p = operator new(1);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete  ( void* ptr ) noexcept;                    (del 1)
    operator delete(p);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new[]( std::size_t count );                       (new 2)
    p = operator new[](1);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete[]( void* ptr ) noexcept;                    (del 2)
    operator delete[](p);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new  ( std::size_t count, std::align_val_t al );  (new 3)
    //     (since C++17)
    const std::align_val_t Alignment{__STDCPP_DEFAULT_NEW_ALIGNMENT__ * 2};
    p = operator new(1, Alignment);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete ( void* ptr, std::align_val_t al );         (del 3)
    //     (since C++17)
    operator delete(p, Alignment);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new[]( std::size_t count, std::align_val_t al );  (new 4)
    //     (since C++17)
    p = operator new[](1, Alignment);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete[]( void* ptr, std::align_val_t al );        (del 4)
    operator delete[](p, Alignment);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // Repeat allocations to test sized-delete overloads.
    // void operator delete  ( void* ptr, std::size_t sz );             (del 5)
    p = operator new(1);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    operator delete(p, 1);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void operator delete[]( void* ptr, std::size_t sz ) noexcept;    (del 6)
    p = operator new[](1);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    operator delete[](p, 1);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void operator delete  ( void* ptr, std::size_t sz,
    //                        std::align_val_t al );                    (del 7)
    p = operator new(1, Alignment);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    operator delete(p, 1, Alignment);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void operator delete[]( void* ptr, std::size_t sz,
    //                        std::align_val_t al );                    (del 8)
    p = operator new[](1, Alignment);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    operator delete[](p, 1, Alignment);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // replaceable non-throwing allocation functions
    // void* operator new  ( std::size_t count, const std::nothrow_t&); (new 5)
    p = operator new(1, std::nothrow);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete  ( void* ptr, const std::nothrow_t& tag );  (del 9)
    operator delete(p, std::nothrow);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new[]( std::size_t count,
    //                       const std::nothrow_t& tag );               (new 6)
    p = operator new[](1, std::nothrow);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete[]( void* ptr, const std::nothrow_t& tag ); (del 10)
    operator delete[](p, std::nothrow);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new  ( std::size_t count, std::align_val_t al,
    //                       const std::nothrow_t& );                   (new 7)
    p = operator new(1, Alignment, std::nothrow);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete  ( void* ptr, std::align_val_t al,
    //                        const std::nothrow_t& tag );             (del 11)
    operator delete(p, Alignment, std::nothrow);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // void* operator new[]( std::size_t count, std::align_val_t al,
    //                       const std::nothrow_t& );                   (new 8)
    p = operator new[](1, Alignment, std::nothrow);
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);
    // void operator delete[]( void* ptr, std::align_val_t al,
    //                        const std::nothrow_t& tag );             (del 12)
    operator delete[](p, Alignment, std::nothrow);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // Test cb_malloc() / cb_free()
    // /////////////////////////////////////////////
    p = static_cast<char*>(cb_malloc(sizeof(char) * 10));
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 10);
    cb_free(p);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // Test cb_realloc() /////////////////////////////////////////////////////
    p = static_cast<char*>(cb_malloc(1));
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 1);

    // Allocator may round up allocation sizes; so it's hard to
    // accurately predict how much cb::ArenaMalloc::getPreciseAllocated(client)
    // will increase. Hence we just increase by a "large" amount and check at
    // least half that increment.
    size_t prev_size = cb::ArenaMalloc::getPreciseAllocated(client);
    p = static_cast<char*>(cb_realloc(p, sizeof(char) * 100));
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), (prev_size + 50));

    prev_size = cb::ArenaMalloc::getPreciseAllocated(client);
    p = static_cast<char*>(cb_realloc(p, 1));
    EXPECT_LT(cb::ArenaMalloc::getPreciseAllocated(client), prev_size);

    prev_size = cb::ArenaMalloc::getPreciseAllocated(client);
    char* q = static_cast<char*>(cb_realloc(nullptr, 10));
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), prev_size + 10);

    cb_free(p);
    cb_free(q);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // Test cb_calloc() //////////////////////////////////////////////////////
    p = static_cast<char*>(cb_calloc(sizeof(char), 20));
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client), 20);
    cb_free(p);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));

    // Test indirect use of malloc() via cb_strdup() /////////////////////////
    p = cb_strdup("random string");
    EXPECT_GE(cb::ArenaMalloc::getPreciseAllocated(client),
              sizeof("random string"));
    cb_free(p);
    EXPECT_EQ(0, cb::ArenaMalloc::getPreciseAllocated(client));
}

// Test that the various memory allocation / deletion functions are correctly
// accounted for, when run in a parallel thread.
TEST_F(MemoryTrackerTest, Accounting) {
    thread = std::thread(&MemoryTrackerTest::AccountingTestThread, this);
    thread.join();
}

/* Test that malloc_usable_size is correctly interposed when using a
 * non-system allocator, as otherwise, our global new replacement could
 * lead to memory being allocated with jemalloc, but the system
 * malloc_usable_size being called with it.
 * We compare the result of malloc_usable_size to the result of
 * AllocHooks::get_allocation_size, which under jemalloc
 * maps to je_malloc_usable_size.
 * If these differ, or this test segfaults, it is suspicious and
 * worth investigating.
 * NB: ASAN is not helpful here as it does not work well with jemalloc
 */
TEST_F(MemoryTrackerTest, mallocUsableSize) {
    // Allocate some data
    auto* ptr = new char[1];

    size_t allocHooksResult = cb::ArenaMalloc::malloc_usable_size(ptr);
    size_t directCallResult = cb_malloc_usable_size(ptr);

    EXPECT_EQ(allocHooksResult, directCallResult);

    delete[] ptr;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
