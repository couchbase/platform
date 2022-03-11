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
#include <platform/awaitable_semaphore.h>
#include <platform/semaphore.h>
#include <platform/semaphore_guard.h>

#include <boost/thread/barrier.hpp>

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
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

TEST(SemaphoreTest, CapacityIncrease) {
    {
        // more tokens are available after a capacity increase
        cb::Semaphore s{2};

        EXPECT_TRUE(s.try_acquire(2)); // 2 held
        EXPECT_FALSE(s.try_acquire()); // no more available
        s.setCapacity(3); // add one token
        EXPECT_TRUE(s.try_acquire()); // can aquire that token
        EXPECT_FALSE(s.try_acquire()); // but no extra
        s.release(3);
    }
    {
        // more tokens are available after a capacity increase
        cb::Semaphore s{2};

        EXPECT_FALSE(s.try_acquire(3)); // 3 is beyond the capacity
        s.setCapacity(3); // add one token
        EXPECT_TRUE(s.try_acquire(3)); // can aquire 3 tokens now
        EXPECT_FALSE(s.try_acquire()); // but no extra
        s.release(3);
    }
}

TEST(SemaphoreTest, CapacityDecrease) {
    {
        // decreasing the capacity reduces how many tokens
        // are available
        cb::Semaphore s{2};

        EXPECT_TRUE(s.try_acquire(2)); // 2 held
        EXPECT_FALSE(s.try_acquire()); // no more available

        // decreasing the number of tokens when the max is already held
        // will drive available tokens negative, but that's okay
        // Once all outstanding tokens are released, availableTokens=capacity
        s.setCapacity(1); // remove one token

        EXPECT_FALSE(s.try_acquire()); // still can't acquire more tokens
        s.release(2);

        EXPECT_FALSE(s.try_acquire(2)); // the new max is respected
        EXPECT_TRUE(s.try_acquire(1)); // only the one token can be acquired
        EXPECT_FALSE(s.try_acquire()); // no extra
        s.release(1);
    }

    {
        // decreasing the capacity reduces how many tokens
        // are available
        cb::Semaphore s{2};

        EXPECT_TRUE(s.try_acquire(1)); // 1 held, 1 available
        s.setCapacity(1); // remove one token

        EXPECT_FALSE(s.try_acquire()); // now can't acquire a second token
        s.release(1);
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

class TestWaiter : public cb::Waiter {
public:
    TestWaiter(std::function<void()> cb) : cb(std::move(cb)) {
    }

    void signal() override {
        cb();
    }

private:
    std::function<void()> cb;
};

TEST(SemaphoreTest, AwaitableAcquireAndRelease) {
    cb::AwaitableSemaphore s{1};

    auto waiter = std::make_shared<TestWaiter>(
            [] { FAIL() << "waiter should not be notified"; });

    EXPECT_TRUE(s.acquire_or_wait(waiter));
    s.release();
}

TEST(SemaphoreTest, AwaitableAcquireWaitsIfNoTokens) {
    cb::AwaitableSemaphore s{1};

    auto waiter1 = std::make_shared<TestWaiter>(
            [] { FAIL() << "first waiter should not be notified"; });

    EXPECT_TRUE(s.acquire_or_wait(waiter1));
    EXPECT_EQ(0, s.getWaiters().size());

    int notificationCount = 0;

    auto waiter2 = std::make_shared<TestWaiter>(
            [&notificationCount] { notificationCount++; });

    EXPECT_FALSE(s.acquire_or_wait(waiter2));

    auto waiters = s.getWaiters();
    EXPECT_EQ(1, waiters.size());

    // the right waiter is queued
    EXPECT_EQ(waiter2, waiters.front().lock());

    // no notification yet
    EXPECT_EQ(0, notificationCount);

    s.release();

    // no one is waiting anymore
    EXPECT_EQ(0, s.getWaiters().size());

    // notified exactly once
    EXPECT_EQ(1, notificationCount);
}

TEST(SemaphoreTest, AwaitableCapacityIncrease) {
    // waiters are notified after capacity increases
    cb::AwaitableSemaphore s{1};

    EXPECT_TRUE(s.try_acquire(1)); // hold the one token

    int notificationCount = 0;

    auto waiter = std::make_shared<TestWaiter>(
            [&notificationCount] { notificationCount++; });

    EXPECT_FALSE(s.acquire_or_wait(waiter));

    auto waiters = s.getWaiters();
    EXPECT_EQ(1, waiters.size());

    // the right waiter is queued
    EXPECT_EQ(waiter, waiters.front().lock());

    // no notification yet
    EXPECT_EQ(0, notificationCount);

    s.setCapacity(10); // add several tokens

    // no waiters anymore
    EXPECT_EQ(0, s.getWaiters().size());

    // notified exactly once
    EXPECT_EQ(1, notificationCount);
}

TEST(SemaphoreTest, AwaitableCapacityDecrease) {
    // decreasing the capacity reduces how many tokens
    // are available
    cb::AwaitableSemaphore s{2};

    EXPECT_TRUE(s.try_acquire()); // 1 held
    EXPECT_TRUE(s.try_acquire()); // 2 held

    int notificationCount = 0;

    auto waiter = std::make_shared<TestWaiter>(
            [&notificationCount] { notificationCount++; });

    EXPECT_FALSE(s.acquire_or_wait(waiter));

    // waiting
    EXPECT_EQ(1, s.getWaiters().size());

    s.setCapacity(1); // remove one token
    // no notification yet
    EXPECT_EQ(0, notificationCount);

    // releasing 1 token _does not_ lead to an available token
    // because the maximum has decreased; the release returned the
    // semaphore to tokens==0.
    // However, for simplicity it _does_ notify the task.
    // a "spurious" notification is possibly wasteful, but is safe.
    // a _missed_ notification would be bad, so err on the side of safety.
    s.release(); // back down to 1 held

    // notified
    EXPECT_EQ(1, notificationCount);

    // no longer waiting
    EXPECT_EQ(0, s.getWaiters().size());

    // task tries to acquire a token again, but still cannot!
    // 1 token is already held, and the current capacity is 1.
    EXPECT_FALSE(s.acquire_or_wait(waiter));

    // back to waiting
    EXPECT_EQ(1, s.getWaiters().size());

    s.release(); // now there's no tokens held

    // waiter had queued itself again, so was notified again
    EXPECT_EQ(2, notificationCount);

    // so the task can now acquire a token
    EXPECT_TRUE(s.acquire_or_wait(waiter));

    // and does not need to wait
    EXPECT_EQ(0, s.getWaiters().size());
    s.release();
}

TEST(SemaphoreTest, AwaitableUniqueWaiters) {
    // test that waiting on a semaphore twice does not queue the waiter
    // for notification twice. in situations analogous to spurious wakeups,
    // it needs to be safe for a waiter to be triggered by "something else"
    // only to try acquire a token again, and fail.
    cb::AwaitableSemaphore s{2};

    EXPECT_TRUE(s.try_acquire(2)); // 2 held

    int notificationCount = 0;

    auto waiter = std::make_shared<TestWaiter>(
            [&notificationCount] { notificationCount++; });

    EXPECT_FALSE(s.acquire_or_wait(waiter));

    // waiting
    auto waiters = s.getWaiters();
    EXPECT_EQ(1, waiters.size());

    // the right waiter is queued
    EXPECT_EQ(waiter, waiters.front().lock());

    // no notification yet
    EXPECT_EQ(0, notificationCount);

    // if the waiter wakes for some other reason, it should try
    // to acquire a token again, and still fail.
    EXPECT_FALSE(s.acquire_or_wait(waiter));

    EXPECT_EQ(1, s.getWaiters().size());

    // now release the tokens
    s.release(2);

    // the waiter should be notified _once_
    EXPECT_EQ(1, notificationCount);

    // and now there's no queued waiters
    EXPECT_EQ(0, s.getWaiters().size());
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

constexpr int numTestTokens = 2;
constexpr int numTestThreads = 10;
constexpr int numTestTasks = 1000;

class SemaphoreTestThread : public cb::Waiter {
public:
    SemaphoreTestThread(boost::barrier& readyBarrier,
                        std::atomic<int>& threadsActive,
                        boost::barrier& doneBarrier,
                        cb::AwaitableSemaphore& testSemaphore)
        : readyBarrier(readyBarrier),
          threadsActive(threadsActive),
          doneBarrier(doneBarrier),
          testSemaphore(testSemaphore) {
    }

    void start() {
        workerThread = std::thread([this] { this->run(); });
    }

    void run() {
        using namespace std::chrono_literals;
        // wait for all the threads to be constructed and ready
        readyBarrier.wait();

        // fake doing `numTestTasks` "tasks" per thread, limited in
        // concurrency by an AwaitableSemaphore
        // (note, a normal, blocking semaphore would be sensible
        // here but this is specifically to test AwaitableSemaphore).
        while (tasksLeft.load() > 0) {
            if (!testSemaphore.acquire_or_wait(shared_from_this())) {
                // couldn't acquire a token yet, and we don't want this
                // thread spinning. Sleep until signalled that tokens are
                // available.
                sleepUntilSignalled();
                continue;
            }
            // only numTestTokens threads should be able to take the semaphore
            // at the same time. This might by chance not catch anything, but
            // check it anyway.
            EXPECT_LE(++threadsActive, numTestTokens);
            // got a token! Don't have any actual "work" to do while holding it
            // but yield to give other threads a chance to
            // exercise the semaphore.
            std::this_thread::yield();

            threadsActive--;

            // now release the semaphore
            testSemaphore.release();
            tasksLeft--;
        }

        doneBarrier.wait();
    }

    void sleepUntilSignalled() {
        auto lh = std::unique_lock(lock);
        condvar.wait(lh, [&] { return signalled.load(); });
        signalled = false;
    }

    void signal() override {
        auto lh = std::unique_lock(lock);
        signalled = true;
        condvar.notify_one();
    }

    bool done() const {
        return tasksLeft.load() == 0;
    }

    std::thread workerThread;

    boost::barrier& readyBarrier;
    std::atomic<int>& threadsActive;
    boost::barrier& doneBarrier;
    cb::AwaitableSemaphore& testSemaphore;

    std::atomic<bool> signalled = false;
    std::mutex lock;
    std::condition_variable condvar;

    std::atomic<int> tasksLeft = numTestTasks;
};

TEST(SemaphoreTest, AwaitableMultiThreaded) {
    // AwaitableSemaphore was designed with tasks in mind - non-blocking
    // (as blocking would take up a thread in a pool), and capable of notifying
    // tasks which wanted to acquire a token but but failed
    // (tasks can be woken and will be executed in the pool "soon").
    // To test with multiple threads but without the task/pool requires
    // a bit of fakery.

    // two "tasks" can run concurrently
    cb::AwaitableSemaphore testSemaphore{size_t(numTestTokens)};
    boost::barrier readyBarrier(numTestThreads);
    std::atomic<int> threadsActive = 0;
    boost::barrier doneBarrier(numTestThreads + 1);

    std::list<std::shared_ptr<SemaphoreTestThread>> threads;

    for (int i = 0; i < numTestThreads; ++i) {
        threads.push_back(std::make_shared<SemaphoreTestThread>(
                readyBarrier, threadsActive, doneBarrier, testSemaphore));
        threads.back()->start();
    }

    doneBarrier.wait();

    // if the threads completed, all the "tasks" were executed.
    // The threads check that it didn't exceed the max
    // concurrency as set by the semaphore.

    for (const auto& thread : threads) {
        thread->workerThread.join();
    }
}

TEST(SemaphoreTest, AwaitableMultiThreaded_ExternalWake) {
    // Test that waiters do not get double-notified if they are woken
    // by something other than the semaphore.

    // only 1 task can run at a time
    cb::AwaitableSemaphore testSemaphore{1};

    // manually acquire the one token - the waiter can't get it yet
    ASSERT_TRUE(testSemaphore.try_acquire());

    struct TestWaiter : public cb::Waiter {
        void signal() override {
            signalled++;
        }

        std::atomic<int> signalled = 0;
    };

    auto waiterA = std::make_shared<TestWaiter>();
    auto waiterB = std::make_shared<TestWaiter>();

    // simulate waiterA "runs" once, and cannot acquire a token
    ASSERT_FALSE(testSemaphore.acquire_or_wait(waiterA));
    // A is now queued for notification

    // simulate waiterB "runs" once, and cannot acquire a token
    ASSERT_FALSE(testSemaphore.acquire_or_wait(waiterB));
    // B is now queued for notification too

    // release the token - this should signal waiterA
    testSemaphore.release();

    ASSERT_EQ(1, waiterA->signalled.load());
    // the task waiterA represents should now try to run "soon"
    // but it may not be instant.

    // What if waiterB is woken by "something else"
    // and tries to acquire a token again, before waiterA does?

    // it should succeed, the token _is_ available
    // _Here_ is where waiterB must be removed from the notification queue.
    ASSERT_TRUE(testSemaphore.acquire_or_wait(waiterB));

    // and when A tries, it should fail, as the token is not available.
    // this will re-queue A for notification.
    ASSERT_FALSE(testSemaphore.acquire_or_wait(waiterA));

    // B later releases the token
    testSemaphore.release();

    // and this notifies A again
    EXPECT_EQ(2, waiterA->signalled.load());

    // _not_ B
    EXPECT_EQ(0, waiterB->signalled.load());
    // note that waiterB didn't get notified at all - some "other thing"
    // coincidentally triggered the task to run again at just the right
    // time. This is _fine_ - it got a turn to do the work it needed to,
    // and didn't drop a notification on the floor.
    // If it had been notified (and ignored it) it would have left waiterA
    // queued for notification, even though a token was now available.
}
