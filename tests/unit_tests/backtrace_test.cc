/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/* Test for print_backtrace - Setup a call stack of (at least) 3 frames,
 * then call print_backtrace; verifying that we get at least 3 frames.
 */
#include <folly/portability/GTest.h>
#include <platform/backtrace.h>
#include <platform/cbassert.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

#ifdef WIN32
    #define NOINLINE __declspec(noinline)
#else
    #define NOINLINE __attribute__((noinline))
#endif

NOINLINE static int leaf();
NOINLINE static int middle();
NOINLINE static int outer();

// Variable used in each function in the chain; to defeat tail-call
// optimization.
static int dummy;

// Count of how many frames we have seen.
static int frames;
static void* expected_ctx = (void*)0xcbdb;

static void write_callback(void* ctx, const char* frame) {
    cb_assert(ctx == expected_ctx);
    cb_assert(frame != nullptr);
    cb_assert(strlen(frame) > 0);
    printf("%s\n", frame);
    frames++;
}

static int leaf() {
    print_backtrace(write_callback, expected_ctx);
    return dummy++;
}

static int middle() {
    leaf();
    return dummy++;
}

static int outer() {
    middle();
    return dummy++;
}

// Test the print_backtrace() function
TEST(BacktraceTest, PrintBacktrace) {
    outer();
    EXPECT_GE(frames, 3);
}

template<typename T>
::testing::AssertionResult ArrayFilledWith(const T expected,
                                           const T* actual,
                                           size_t size) {
    for (size_t i(0); i < size; ++i) {
        if (expected != actual[i]) {
            return ::testing::AssertionFailure()
                << "array[" << i << "] (" << +actual[i]
                << ") != expected (" << +expected << ")";
        }
    }

    return ::testing::AssertionSuccess();
}

// Regression test for MB-19580 - print_backtrace_to_buffer has
// incorrect buffer checking.
TEST(BackTraceTest, PrintBacktraceToBufferMB19580) {
    // The original issue manifested as crashing the process, however
    // for extra resilience we create redzones around the buffer and
    // check that it isn't touched (instead of relying on possibly
    // crashing the process).
    const size_t redzone_sz{1024};
    const size_t buffer_sz{8};
    char data[redzone_sz + buffer_sz + redzone_sz];

    // Set the whole data array to to known, non-zero value. (0xee)
    std::fill_n(data, sizeof(data), 0xee);

    // We expect the backtrace to overrun our small buffer.
    char* buffer_ptr = data + redzone_sz;
    EXPECT_FALSE(print_backtrace_to_buffer("", buffer_ptr, buffer_sz));

    // Redzones should not be touched.
    const char* redzone_1 = data;
    const char* redzone_2 = redzone_1 + redzone_sz + buffer_sz;
    EXPECT_TRUE(ArrayFilledWith(char(0xee), redzone_1, redzone_sz));
    EXPECT_TRUE(ArrayFilledWith(char(0xee), redzone_2, redzone_sz));
}

TEST(BackTraceTest, Current) {
    auto backtrace = cb::backtrace::current();
#if defined(WIN32) || defined(__APPLE__)
    // Arg.. our cv don't give us full symbols on g++ builds...
    EXPECT_TRUE(backtrace.contains("BackTraceTest")) << backtrace;
#else
    EXPECT_TRUE(backtrace.contains("platform_unit_test")) << backtrace;
#endif
}
