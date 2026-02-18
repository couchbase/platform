/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/cb_time.h>
#include <platform/token_bucket_rate_limiter.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

TEST(TokenBucketRateLimiterTest, Construction) {
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;
    // First call to getAvailableTokens initializes the bucket to capacity
    EXPECT_EQ(1000, limiter.getAvailableTokens(1000));
}

TEST(TokenBucketRateLimiterTest, ZeroBytesDoesNotBlock) {
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;
    // Initialize the bucket
    (void)limiter.getAvailableTokens(1000);
    // Should return immediately for zero bytes
    limiter.acquire(0, 1000);
    // Tokens unchanged (still 1000)
    EXPECT_EQ(1000, limiter.getAvailableTokens(1000));
}

TEST(TokenBucketRateLimiterTest, AcquireConsumesTokens) {
    cb::time::StaticClockGuard clockGuard;

    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    limiter.acquire(100, 1000);
    EXPECT_EQ(900, limiter.getAvailableTokens(1000));

    limiter.acquire(200, 1000);
    EXPECT_EQ(700, limiter.getAvailableTokens(1000));
}

TEST(TokenBucketRateLimiterTest, TokensRefillOverTime) {
    cb::time::StaticClockGuard clockGuard;

    // 1000 bytes per second = 1 byte per millisecond
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    // Consume all tokens
    limiter.acquire(1000, 1000);
    EXPECT_EQ(0, limiter.getAvailableTokens(1000));

    // Advance clock by 500ms - should refill 500 tokens
    cb::time::steady_clock::advance(500ms);
    EXPECT_EQ(500, limiter.getAvailableTokens(1000));

    // Advance by another 500ms - should be full again
    cb::time::steady_clock::advance(500ms);
    EXPECT_EQ(1000, limiter.getAvailableTokens(1000));
}

TEST(TokenBucketRateLimiterTest, TokensCappedAtRate) {
    cb::time::StaticClockGuard clockGuard;

    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    // Start with full bucket by acquiring with zero bytes
    limiter.acquire(0, 1000);
    EXPECT_EQ(1000, limiter.getAvailableTokens(1000));

    // Already full, advance time
    cb::time::steady_clock::advance(5s);

    // Should still be capped at 1000 (the rate)
    EXPECT_EQ(1000, limiter.getAvailableTokens(1000));
}

TEST(TokenBucketRateLimiterTest, BlocksWhenInsufficientTokens) {
    // Use real clock for blocking tests - wait_for uses real time
    // 10000 bytes per second = 10 bytes per millisecond
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    // Fill the bucket initially
    limiter.acquire(0, 10000);

    // Consume all tokens
    limiter.acquire(10000, 10000);

    auto startTime = std::chrono::steady_clock::now();

    // Acquire 500 bytes - should take ~50ms to refill
    limiter.acquire(500, 10000);

    auto elapsed = std::chrono::steady_clock::now() - startTime;

    // Should have waited approximately 50ms (allow some tolerance)
    EXPECT_GE(elapsed, 40ms);
    EXPECT_LT(elapsed, 200ms);
}

TEST(TokenBucketRateLimiterTest, FIFOOrdering) {
    // Use real clock for blocking tests - wait_for uses real time
    // 10000 bytes per second = 10 bytes per millisecond
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    // Fill and then consume all tokens
    limiter.acquire(0, 10000);
    limiter.acquire(10000, 10000);

    std::vector<int> completionOrder;
    std::mutex orderMutex;

    // Start three threads that want different amounts
    // Thread 1 wants 100 bytes (arrives first) - needs 10ms
    // Thread 2 wants 50 bytes (arrives second) - needs 5ms
    // Thread 3 wants 100 bytes (arrives third) - needs 10ms
    // Even though thread 2 wants fewer bytes, it should complete after thread 1

    std::thread t1([&] {
        limiter.acquire(100, 10000);
        std::lock_guard<std::mutex> lock(orderMutex);
        completionOrder.push_back(1);
    });

    std::this_thread::sleep_for(5ms); // Ensure t1 gets ticket first

    std::thread t2([&] {
        limiter.acquire(50, 10000);
        std::lock_guard<std::mutex> lock(orderMutex);
        completionOrder.push_back(2);
    });

    std::this_thread::sleep_for(5ms); // Ensure t2 gets ticket second

    std::thread t3([&] {
        limiter.acquire(100, 10000);
        std::lock_guard<std::mutex> lock(orderMutex);
        completionOrder.push_back(3);
    });

    t1.join();
    t2.join();
    t3.join();

    // Verify FIFO ordering was maintained
    ASSERT_EQ(3, completionOrder.size());
    EXPECT_EQ(1, completionOrder[0]);
    EXPECT_EQ(2, completionOrder[1]);
    EXPECT_EQ(3, completionOrder[2]);
}

TEST(TokenBucketRateLimiterTest, MillisecondsRateUnit) {
    cb::time::StaticClockGuard clockGuard;

    // 10 bytes per millisecond = 10000 bytes per second
    cb::TokenBucketRateLimiter<std::chrono::milliseconds> limiter;

    // Fill the bucket
    limiter.acquire(0, 10);

    EXPECT_EQ(10, limiter.getAvailableTokens(10));

    // Consume all
    limiter.acquire(10, 10);
    EXPECT_EQ(0, limiter.getAvailableTokens(10));

    // Advance 5ms - should refill 50 tokens, but capped at 10
    cb::time::steady_clock::advance(5ms);
    EXPECT_EQ(10, limiter.getAvailableTokens(10));
}

TEST(TokenBucketRateLimiterTest, MinutesRateUnit) {
    cb::time::StaticClockGuard clockGuard;

    // 6000 bytes per minute = 100 bytes per second
    cb::TokenBucketRateLimiter<std::chrono::minutes> limiter;

    // Consume all tokens (initially fills to 6000)
    limiter.acquire(6000, 6000);
    EXPECT_EQ(0, limiter.getAvailableTokens(6000));

    // Advance 1 second - should refill 100 tokens
    cb::time::steady_clock::advance(1s);
    EXPECT_EQ(100, limiter.getAvailableTokens(6000));
}

TEST(TokenBucketRateLimiterTest, MultiThreadedThroughput) {
    // Test that rate limiting actually works by measuring throughput
    constexpr size_t bytesPerSecond = 10000;
    constexpr size_t numThreads = 4;
    constexpr size_t bytesPerThread = 500;
    constexpr size_t chunkSize = 100;

    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    auto startTime = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([&limiter, bytesPerThread, chunkSize] {
            size_t remaining = bytesPerThread;
            while (remaining > 0) {
                size_t toAcquire = std::min(chunkSize, remaining);
                limiter.acquire(toAcquire, bytesPerSecond);
                remaining -= toAcquire;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = endTime - startTime;

    // Total bytes = 4 * 500 = 2000 bytes
    // At 10000 bytes/sec with initial bucket of 10000, first 2000 bytes
    // should be near-instant (from bucket). So elapsed should be small.
    // This test mainly validates thread safety under TSAN.

    // Should complete in reasonable time (< 1 second since all fits in bucket)
    EXPECT_LT(elapsed, 1s);
}

// This test is disabled by default as it takes ~3 seconds of real time.
// Enable it for thorough manual testing of actual rate limiting behavior.
TEST(TokenBucketRateLimiterTest, DISABLED_MultiThreadedWithThrottling) {
    // Test with actual throttling - request more than bucket capacity
    constexpr size_t bytesPerSecond = 1000;
    constexpr size_t numThreads = 4;
    constexpr size_t bytesPerThread = 1000; // Total 4000 bytes
    constexpr size_t chunkSize = 100;

    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    auto startTime = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    std::atomic<size_t> totalAcquired{0};

    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back(
                [&limiter, &totalAcquired, bytesPerThread, chunkSize] {
                    size_t remaining = bytesPerThread;
                    while (remaining > 0) {
                        size_t toAcquire = std::min(chunkSize, remaining);
                        limiter.acquire(toAcquire, bytesPerSecond);
                        totalAcquired.fetch_add(toAcquire,
                                                std::memory_order_relaxed);
                        remaining -= toAcquire;
                    }
                });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           endTime - startTime)
                           .count();

    // All bytes should have been acquired
    EXPECT_EQ(numThreads * bytesPerThread, totalAcquired.load());

    // 4000 bytes at 1000 bytes/sec = 4 seconds minimum
    // But we start with 1000 tokens in bucket, so need 3 seconds of refill
    // Expect at least 2.5 seconds (allowing tolerance)
    EXPECT_GE(elapsed, 2500);
}

TEST(TokenBucketRateLimiterTest, LargeAcquireBlocksUntilSufficientTokens) {
    // Use real clock for blocking tests - wait_for uses real time
    // 10000 bytes per second = 10 bytes per millisecond
    cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;

    // Fill and drain the bucket
    limiter.acquire(0, 10000);
    limiter.acquire(10000, 10000);

    auto startTime = std::chrono::steady_clock::now();

    // Acquire 800 bytes - should take ~80ms to refill
    limiter.acquire(800, 10000);

    auto elapsed = std::chrono::steady_clock::now() - startTime;

    // Should have waited approximately 80ms (allow some tolerance)
    EXPECT_GE(elapsed, 70ms);
    EXPECT_LT(elapsed, 200ms);
}
