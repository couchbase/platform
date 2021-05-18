/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/rwlock.h>

#include <folly/SharedMutex.h>
#include <folly/portability/GTest.h>
#include <shared_mutex>

/**
 * Templated death test which tests the interoperability of two lock types.
 * When run under TSan (they are skipped otherwise), tests should crash due to
 * TSan detecting the expected threading errors.
 *
 * @tparam LOCKS The two locks to test. Should expose LockA and LockB nested
 * types.
 */
template <typename Locks>
class LockDeathTest : public ::testing::Test {
public:
    void SetUp() override {
        // Check out preconditions - require that TSan is configured to halt
        // immediately on an error.
        auto* tsanOptions = getenv("TSAN_OPTIONS");
        ASSERT_TRUE(tsanOptions) << "LockDeathTests require that TSAN_OPTIONS "
                                    "is defined (and contains "
                                    "'halt_on_error=1')";
        ASSERT_TRUE(std::string(tsanOptions).find("halt_on_error=1") !=
                    std::string::npos)
                << "LockDeathTests require that ThreadSanitizer is run with "
                   "'halt_on_error' enabled. Check that the TSAN_OPTIONS env "
                   "var contains 'halt_on_error=1'";
    }
    typename Locks::LockA lockA;
    typename Locks::LockB lockB;
};

/// Subclass for exclusive locks
template <typename Locks>
class ExclusiveLockDeathTest : public LockDeathTest<Locks> {};

/// Subclass for shared (reader-writer) locks
template <typename Locks>
class SharedLockDeathTest : public LockDeathTest<Locks> {};

template <typename A, typename B>
struct LockPair {
    using LockA = A;
    using LockB = B;
};

/// Lock types to test for exclusive access.
// macOS's C++ standard library doesn't appear to have annotations
// for std::mutex etc - so TSan fails to detect lock-order issues.
// As such some combinations cannot be checked on macOS.
using LockTypes = ::testing::Types<
// Identity tests (same - same)
#if !defined(__APPLE__)
        LockPair<std::mutex, std::mutex>,
        LockPair<std::shared_timed_mutex, std::shared_timed_mutex>,
        LockPair<folly::SharedMutex, folly::SharedMutex>,
#endif
        LockPair<cb::RWLock, cb::RWLock>,

// Each mutex with each other mutex, in both orders (A,B) & (B,A).
// While you'd _think_ that it would be sufficient to just check each
// pair once, given in the test we call:
//     A.lock(),
//     B.lock(),
//     unlock
//     ...
//     B.lock(),
//     A.lock()
// It turns out that on macOS at least TSan can detect lock inversions
// in _one_ order but not the other -  ¯\_(ツ)_/¯
#if !defined(__APPLE__)
        LockPair<std::mutex, std::shared_timed_mutex>,
        LockPair<std::mutex, cb::RWLock>,
        LockPair<std::mutex, folly::SharedMutex>,

        LockPair<std::shared_timed_mutex, std::mutex>,
        LockPair<std::shared_timed_mutex, cb::RWLock>,
        LockPair<std::shared_timed_mutex, folly::SharedMutex>,
#endif

        LockPair<cb::RWLock, std::mutex>,
#if !defined(__APPLE__)
        LockPair<cb::RWLock, std::shared_timed_mutex>,
#endif
        LockPair<cb::RWLock, folly::SharedMutex>,

        LockPair<folly::SharedMutex, std::mutex>,
#if !defined(__APPLE__)
        LockPair<folly::SharedMutex, std::shared_timed_mutex>,
#endif
        LockPair<folly::SharedMutex, cb::RWLock>>;

/// Lock types to test for shared access
using SharedLockTypes = ::testing::Types<
#if !defined(__APPLE__)
        LockPair<std::shared_timed_mutex, std::shared_timed_mutex>,
        LockPair<cb::RWLock, cb::RWLock>,
        LockPair<cb::RWLock, std::shared_timed_mutex>,
        LockPair<cb::RWLock, folly::SharedMutex>,
        LockPair<folly::SharedMutex, std::shared_timed_mutex>,
#endif // defined(__APPLE__)
        LockPair<folly::SharedMutex, folly::SharedMutex>>;

TYPED_TEST_SUITE(ExclusiveLockDeathTest, LockTypes);
TYPED_TEST_SUITE(SharedLockDeathTest, SharedLockTypes);

// The following tests all rely on ThreadSanitizer (they are checking
// that our different locks interact correctly with TSan)
#if defined(THREAD_SANITIZER)

// Acquire Lock1 and Lock2, and release; then acquire in opposite order.
// Expect TSan to report a lock inversion order.
TYPED_TEST(ExclusiveLockDeathTest, LockOrderInversion12) {
    typename TypeParam::LockA lock1;
    typename TypeParam::LockB lock2;

    ASSERT_DEATH(
            {
                lock1.lock();
                lock2.lock();
                lock2.unlock();
                lock1.unlock();

                lock2.lock();
                lock1.lock();
                lock1.unlock();
                lock2.unlock();
            },
            ".*WARNING: ThreadSanitizer: lock-order-inversion .*");
}

// Acquire Lock1(read) and Lock2(write), and release; then acquire in opposite
// order.
// Expect TSan to report a lock inversion order.
TYPED_TEST(SharedLockDeathTest, RWInversion) {
    typename TypeParam::LockA lock1;
    typename TypeParam::LockB lock2;

    ASSERT_DEATH(
            {
                lock1.lock_shared();
                lock2.lock();
                lock2.unlock();
                lock1.unlock_shared();

                lock2.lock();
                lock1.lock_shared();
                lock1.unlock_shared();
                lock2.unlock();
            },
            ".*WARNING: ThreadSanitizer: lock-order-inversion .*");
}

// Acquire Lock1(write) and Lock2(read), and release; then acquire in opposite
// order.
// Expect TSan to report a lock inversion order.
TYPED_TEST(SharedLockDeathTest, WRInversion) {
    typename TypeParam::LockA lock1;
    typename TypeParam::LockB lock2;

    ASSERT_DEATH(
            {
                lock1.lock();
                lock2.lock_shared();
                lock2.unlock_shared();
                lock1.unlock();

                lock2.lock_shared();
                lock1.lock();
                lock1.unlock();
                lock2.unlock_shared();
            },
            ".*WARNING: ThreadSanitizer: lock-order-inversion .*");
}

#endif // defined(THREAD_SANITIZER)
