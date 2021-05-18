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