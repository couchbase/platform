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
    void SetUp() {
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
using LockTypes = ::testing::Types<
// macOS's C++ standard library doesn't appear to have annotations
// for std::mutex etc - so TSan fails to detect lock-order issues.
// Only test these mutext types on non-macOS.
#if !defined(__APPLE__)
        // Identity tests (same - same)
        LockPair<std::mutex, std::mutex>,
        LockPair<std::shared_timed_mutex, std::shared_timed_mutex>,
        LockPair<cb::RWLock, cb::RWLock>,

        // Combination of each mutex type with every other.
        LockPair<std::mutex, std::shared_timed_mutex>,
        LockPair<std::mutex, cb::RWLock>,
        LockPair<std::mutex, folly::SharedMutex>,
        LockPair<cb::RWLock, std::shared_timed_mutex>,
        LockPair<cb::RWLock, folly::SharedMutex>,
        LockPair<folly::SharedMutex, std::shared_timed_mutex>,
#endif // defined(__APPLE__)
        // Curiously, macOS _can_ detect issues with
        // <folly::SharedMutex,std::mutex>, but not in the reverse order
        // <std::mutex, folly::SharedMutex>
        LockPair<folly::SharedMutex, std::mutex>,
        LockPair<folly::SharedMutex, folly::SharedMutex>>;

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

TYPED_TEST_CASE(ExclusiveLockDeathTest, LockTypes);
TYPED_TEST_CASE(SharedLockDeathTest, SharedLockTypes);

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
