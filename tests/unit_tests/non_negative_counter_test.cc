/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/non_negative_counter.h>
#include <platform/backtrace.h>
#include <folly/portability/GTest.h>

/**
 * Test fixture for tests checking basic NonNegativeCounter test functionality
 * (unrelated to underflow - hence not specifying UnderflowPolicy).
 */
template <typename T>
class NonNegativeCounterTest : public testing::Test {
};

using CounterTypes = ::testing::Types<cb::NonNegativeCounter<size_t>,
                                      cb::AtomicNonNegativeCounter<size_t>>;
TYPED_TEST_SUITE(NonNegativeCounterTest, CounterTypes);

TYPED_TEST(NonNegativeCounterTest, Increment) {
    TypeParam nnAtomic(1);
    ASSERT_EQ(1u, nnAtomic);

    EXPECT_EQ(2u, ++nnAtomic);
    EXPECT_EQ(2u, nnAtomic++);
    EXPECT_EQ(3u, nnAtomic);
}

TYPED_TEST(NonNegativeCounterTest, Add) {
    TypeParam nnAtomic(1);
    ASSERT_EQ(1u, nnAtomic);

    EXPECT_EQ(3u, nnAtomic += 2);
    EXPECT_EQ(3u, nnAtomic.fetch_add(2));
    EXPECT_EQ(5u, nnAtomic);

    // Adding a negative should subtract from the value
    EXPECT_EQ(5u, nnAtomic.fetch_add(-2));
    EXPECT_EQ(3u, nnAtomic);

    EXPECT_EQ(3u, nnAtomic.fetch_add(-3));
    EXPECT_EQ(0u, nnAtomic);
}

TYPED_TEST(NonNegativeCounterTest, Decrement) {
    TypeParam nnAtomic(2);
    ASSERT_EQ(2u, nnAtomic);

    EXPECT_EQ(1u, --nnAtomic);
    EXPECT_EQ(1u, nnAtomic--);
    EXPECT_EQ(0u, nnAtomic);
}

TYPED_TEST(NonNegativeCounterTest, Subtract) {
    TypeParam nnAtomic(4);
    ASSERT_EQ(4u, nnAtomic);

    EXPECT_EQ(2u, nnAtomic -= 2);
    EXPECT_EQ(2u, nnAtomic.fetch_sub(2));
    EXPECT_EQ(0u, nnAtomic);

    EXPECT_EQ(2u, nnAtomic -= -2);
    EXPECT_EQ(2u, nnAtomic.fetch_sub(-2));
    EXPECT_EQ(4u, nnAtomic);
}

/// Test fixture for tests testing ClampToZero underflow policy.
template <typename T>
class NonNegativeCounterClampAtZeroTest : public testing::Test {};

using ClampAtZeroTypes = ::testing::Types<
        cb::NonNegativeCounter<size_t, cb::ClampAtZeroUnderflowPolicy>,
        cb::AtomicNonNegativeCounter<size_t, cb::ClampAtZeroUnderflowPolicy>>;
TYPED_TEST_SUITE(NonNegativeCounterClampAtZeroTest, ClampAtZeroTypes);

// Test that a NonNegativeCounter will clamp to zero.
TYPED_TEST(NonNegativeCounterClampAtZeroTest, ClampsToZero) {
    TypeParam nnAtomic(0);

    EXPECT_EQ(0u, --nnAtomic);
    EXPECT_EQ(0u, nnAtomic--);
    EXPECT_EQ(0u, nnAtomic);

    nnAtomic = 5;
    EXPECT_EQ(5u, nnAtomic.fetch_sub(10)); // returns previous value
    EXPECT_EQ(0u, nnAtomic); // has been clamped to zero

    nnAtomic = 5;
    EXPECT_EQ(5u, nnAtomic.fetch_add(-10)); // return previous value
    EXPECT_EQ(0u, nnAtomic); // has been clamped to zero
}

// Test that attempting to construct or assign a negative value is rejected.
TYPED_TEST(NonNegativeCounterClampAtZeroTest, ClampsToZeroAssignment) {
    TypeParam nnAtomic(-1);
    EXPECT_EQ(0u, nnAtomic) << "Construction with of negative number should clamped to zero";

    // Reset to different value before next test.
    nnAtomic = 10;

    nnAtomic = -2;
    EXPECT_EQ(0u, nnAtomic) << "Assignment of negative number should have been clamped to zero";
}

/// Test fixture for tests testing ThrowException underflow policy.
template <typename T>
class NonNegativeCounterThrowExceptionTest : public testing::Test {};

using ThrowExceptionTypes = ::testing::Types<
        cb::NonNegativeCounter<size_t, cb::ThrowExceptionUnderflowPolicy>,
        cb::AtomicNonNegativeCounter<size_t, cb::ThrowExceptionUnderflowPolicy>>;
TYPED_TEST_SUITE(NonNegativeCounterThrowExceptionTest, ThrowExceptionTypes);

// Test the ThrowException policy.
TYPED_TEST(NonNegativeCounterThrowExceptionTest, ThrowExceptionPolicy) {
    TypeParam nnAtomic(0);

    EXPECT_THROW(--nnAtomic, std::underflow_error);
    EXPECT_EQ(0u, nnAtomic);
    EXPECT_THROW(nnAtomic--, std::underflow_error);
    EXPECT_EQ(0u, nnAtomic);

    EXPECT_THROW(nnAtomic.fetch_add(-1), std::underflow_error);
    EXPECT_EQ(0u, nnAtomic);

    EXPECT_THROW(nnAtomic += -1, std::underflow_error);
    EXPECT_EQ(0u, nnAtomic);

    EXPECT_THROW(nnAtomic -= 2, std::underflow_error);
    EXPECT_EQ(0u, nnAtomic);
}

// Test that ThrowException policy throws an exception which records where
// the exception was thrown from.
TYPED_TEST(NonNegativeCounterThrowExceptionTest, Backtrace) {
    TypeParam nnAtomic(0);
    try {
        cb::backtrace::initialize();
    } catch (const std::exception& exception) {
        FAIL() << "Failed to initialize backtrace: " << exception.what();
    }

    try {
        --nnAtomic;
    } catch (const std::underflow_error& e) {
        const auto* st = cb::getBacktrace(e);
        ASSERT_TRUE(st);
        // MB-44173: print_backtrace doesn't symbolify for Windows.

        // Hard to accurately predict what we'll see in the backtrace;
        // just check it contains the executable name somewhere
        std::string backtrace;
        print_backtrace_frames(*st, [&backtrace](const char* frame) {
                backtrace += frame;
                backtrace += '\n';
            });
        EXPECT_NE(std::string::npos, backtrace.find("platform_unit_tests"))
            << "when verifying exception backtrace: " << backtrace;
    }
}

// Test that attempting to construct or assign a negative value is rejected.
TYPED_TEST(NonNegativeCounterThrowExceptionTest, Assignment) {
    EXPECT_THROW(TypeParam(-1), std::underflow_error) << "Construction with of negative number should throw";

    TypeParam nnAtomic(10);
    EXPECT_THROW(nnAtomic = -2, std::underflow_error) << "Assignment of negative number should throw";
}
