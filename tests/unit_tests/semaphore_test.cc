/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/semaphore.h>
#include <platform/semaphore_guard.h>

#include <thread>

TEST(SemaphoreTest, AcquireAndRelease) {
    cb::Semaphore s{1};

    EXPECT_TRUE(s.try_acquire());
    s.release();
}

TEST(SemaphoreTest, AcquireFailsIfNoTokens) {
    {
        cb::Semaphore s{1};

        EXPECT_TRUE(s.try_acquire());
        EXPECT_FALSE(s.try_acquire());
        s.release();
    }

    {
        cb::Semaphore s{2};

        EXPECT_TRUE(s.try_acquire());
        EXPECT_TRUE(s.try_acquire());
        EXPECT_FALSE(s.try_acquire());
        s.release();
        s.release();
    }
}

TEST(SemaphoreTest, AcquireMultiple) {
    {
        // acquiring multiple succeeds if sufficient tokens available
        cb::Semaphore s{4};

        EXPECT_TRUE(s.try_acquire(4));
        EXPECT_FALSE(s.try_acquire());
        s.release(4);
    }
    {
        // acquiring multiple succeeds if sufficient tokens available
        cb::Semaphore s{4};

        EXPECT_TRUE(s.try_acquire());
        EXPECT_TRUE(s.try_acquire(2));
        EXPECT_TRUE(s.try_acquire());

        EXPECT_FALSE(s.try_acquire());
        s.release(4);
    }
    {
        // trying to acquire more tokens than available fails
        cb::Semaphore s{2};

        // only 2 available, 3 is too many
        EXPECT_FALSE(s.try_acquire(3));
        EXPECT_TRUE(s.try_acquire());

        // now only 1 available, 3 is still too many
        EXPECT_FALSE(s.try_acquire(3));
        // so is 2
        EXPECT_FALSE(s.try_acquire(2));
        // but acquiring 1 succeeds
        EXPECT_TRUE(s.try_acquire());
        s.release(2);
    }
}

TEST(SemaphoreTest, MultiThreaded) {
    // manipulate a semaphore from multiple threads to try expose any issues
    // under TSAN.
    using namespace std::chrono_literals;
    cb::Semaphore s{2};

    // acquire all tokens immediately
    EXPECT_TRUE(s.try_acquire(2));

    std::vector<std::thread> threads;

    for (int i = 0; i < 4; i++) {
        // create 4 threads which try to acquire and release a token over and
        // over. There are more threads than tokens.
        threads.emplace_back([&s] {
            for (int j = 0; j < 10000; j++) {
                // keep trying to get a token
                while (!s.try_acquire()) {
                    std::this_thread::yield();
                }
                s.release();
                std::this_thread::yield();
            }
        });
    }

    // now all the threads have been created, release the tokens so they
    // can all contend for them.
    s.release(2);

    // join all the threads
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST(SemaphoreTest, Guard) {
    cb::Semaphore s{1};

    {
        // try acquire one token with an RAII guard
        cb::SemaphoreGuard guard(&s, 1);
        EXPECT_TRUE(guard);

        // confirm that no more tokens are available
        EXPECT_FALSE(s.try_acquire(1));
        // guard scope ends
    }

    // confirm token is available again
    EXPECT_TRUE(s.try_acquire(1));
    s.release(1);
}

TEST(SemaphoreTest, GuardMultiple) {
    cb::Semaphore s{3};

    {
        // try acquire two tokens with an RAII guard
        cb::SemaphoreGuard guard(&s, 2);
        EXPECT_TRUE(guard);

        // confirm that only one tokens remain (3 - 2 = 1)
        EXPECT_FALSE(s.try_acquire(2));
        EXPECT_TRUE(s.try_acquire(1));
        s.release(1);
        // guard scope ends
    }

    // confirm all tokens available again
    EXPECT_TRUE(s.try_acquire(3));
    s.release(3);
}

TEST(SemaphoreTest, GuardFailure) {
    cb::Semaphore s{1};

    {
        // try acquire one token with an RAII guard
        cb::SemaphoreGuard guard(&s);
        EXPECT_TRUE(guard);

        // trying to acquire more fails
        cb::SemaphoreGuard guard2(&s);
        EXPECT_FALSE(guard2);

        // directly acquiring with try_acquire also fails
        EXPECT_FALSE(s.try_acquire(1));

        // guard scope ends
    }

    // confirm exactly one token is available again
    EXPECT_FALSE(s.try_acquire(2));
    EXPECT_TRUE(s.try_acquire(1));
    s.release(1);
}

TEST(SemaphoreTest, GuardMove) {
    cb::Semaphore s{1};

    {
        // default construct a guard, no tokens managed
        cb::SemaphoreGuard guardOuter;

        {
            // try acquire one token with an RAII guard
            cb::SemaphoreGuard guard(&s);
            EXPECT_TRUE(guard);

            // trying to acquire more fails
            EXPECT_FALSE(s.try_acquire(1));

            // move the guard
            guardOuter = std::move(guard);
            // guard scope ends, but no tokens released as
            // the guard has been moved out of
        }

        // trying to acquire token still fails, the guard still exists
        EXPECT_FALSE(s.try_acquire(1));
    }

    // confirm exactly one token is available again
    EXPECT_FALSE(s.try_acquire(2));
    EXPECT_TRUE(s.try_acquire(1));
    s.release(1);
}

TEST(SemaphoreTest, GuardRelease) {
    cb::Semaphore s{1};

    {
        // try acquire one token with an RAII guard
        cb::SemaphoreGuard guard(&s);
        EXPECT_TRUE(guard);

        // trying to acquire more fails
        EXPECT_FALSE(s.try_acquire(1));

        // release the token managed by the guard
        // similar semantics to a unique ptr release -
        // resource is not "freed" (returned to semaphore) but must now be
        // managed by the caller
        guard.release();

        // trying to acquire more fails, the token is still held
        // but the guard is no longer responsible
        EXPECT_FALSE(s.try_acquire(1));
        // guard scope ends, but no tokens released as
        // the guard has been released
    }

    // trying to acquire token still fails, destroying the guard
    // does nothing as it has already been released.

    EXPECT_FALSE(s.try_acquire(1));

    // release the token for which the caller became responsible
    s.release(1);
}

TEST(SemaphoreTest, GuardReset) {
    cb::Semaphore s{1};

    {
        // try acquire one token with an RAII guard
        cb::SemaphoreGuard guard(&s);
        EXPECT_TRUE(guard);

        // trying to acquire more fails
        EXPECT_FALSE(s.try_acquire(1));

        // reset the token managed by the guard
        // similar semantics to a unique ptr reset -
        // resource is "freed" (returned to semaphore)
        // and the guard then manages nothing.
        guard.reset();

        // trying to acquire more succeeds, the token has been returned
        EXPECT_TRUE(s.try_acquire(1));
        // guard scope ends, but no tokens released as
        // the guard has been reset
    }

    // guard destruction didn't erroneously return more tokens,
    // no more can be acquired
    EXPECT_FALSE(s.try_acquire(1));

    // release the one token acquired manually earlier
    s.release(1);
}
TEST(SemaphoreTest, GuardShared) {
    auto s = std::make_shared<cb::Semaphore>(1);

    {
        // try acquire one token with an RAII guard
        cb::SemaphoreGuard guard(s);
        EXPECT_TRUE(guard);
        // guard scope ends
    }

    {
        // try acquire one token with an RAII guard, token should
        // be available as previous guard has been destroyed
        cb::SemaphoreGuard guard(s);
        EXPECT_TRUE(guard);

        // reset the shared ptr, when the guard is destroyed
        // tokens should be returned to the semaphore,
        // then the semaphore destroyed as there are no other
        // owners. Attempting to provoke asan failures if
        // the guard did not take a shared ptr.
        s.reset();
    }
}
