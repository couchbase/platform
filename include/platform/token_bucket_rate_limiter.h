/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/cb_time.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace cb {

/**
 * A thread-safe blocking rate limiter using the token bucket algorithm.
 *
 * This class throttles threads to a specified bytes/duration rate. The rate is
 * specified dynamically on each acquire() call, allowing for dynamic adjustment
 * of the throttle rate. When threads call acquire(), they will be blocked if
 * the rate has been exceeded until sufficient tokens (bytes) are available.
 * Threads are served in FIFO order using a ticket system.
 *
 * The token bucket refills continuously at the configured rate. Unlike a
 * simple rate limiter that returns sleep durations, this class actively holds
 * threads using std::condition_variable until they can proceed.
 *
 * On the first acquire() call, the bucket is populated to the capacity
 * (bytesPerPeriod). This allows for an initial burst of up to one period's
 * worth of data.
 *
 * Example usage:
 * @code
 *   // Create a rate limiter
 *   cb::TokenBucketRateLimiter<std::chrono::seconds> limiter;
 *
 *   // Thread calls acquire before writing data, specifying the rate
 *   limiter.acquire(bytesToWrite, 1024 * 1024);
 *   // ... perform write ...
 * @endcode
 *
 * @tparam RateUnit The time unit for the rate (e.g., std::chrono::seconds
 *         means the rate is in bytes per second)
 * @tparam Clock The clock type to use for timing. Defaults to
 *         cb::time::steady_clock which supports static time for testing.
 */
template <typename RateUnit = std::chrono::seconds,
          typename Clock = cb::time::steady_clock>
class TokenBucketRateLimiter {
public:
    using Duration = typename Clock::duration;
    using TimePoint = typename Clock::time_point;

    /**
     * Construct a rate limiter.
     *
     * The rate is specified dynamically on each call to acquire(), which
     * allows for dynamic adjustment of the throttle rate.
     */
    TokenBucketRateLimiter() = default;

    /**
     * Acquire permission to process the specified number of bytes.
     *
     * This method blocks the calling thread until sufficient tokens are
     * available. Threads are served in FIFO order - a thread that called
     * acquire() earlier will be served before one that called later, even
     * if the later thread requests fewer bytes.
     *
     * @param bytes The number of bytes the caller intends to write/process.
     * @param bytesPerPeriod The target rate in bytes per RateUnit.
     *        This is also used as the bucket capacity, allowing bursts up
     *        to one period's worth of data. Must be greater than zero.
     */
    void acquire(size_t bytes, size_t bytesPerPeriod) {
        if (bytes == 0 || bytesPerPeriod == 0) {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex);

        // Get a ticket to ensure FIFO ordering
        const auto myTicket = nextTicket++;

        // Wait until it's our turn AND we have enough tokens. (this also
        // absorbs spurious wake-ups)
        while (true) {
            // Must be our turn first (FIFO)
            if (myTicket == servingTicket) {
                // Refill tokens based on elapsed time
                refillTokens(bytesPerPeriod);
                // Check if we have enough tokens
                if (availableTokens >= bytes) {
                    break;
                }
                // Wait for the calculated duration needed to accumulate
                // sufficient tokens. Note: wait_for uses real time regardless
                // of the Clock template parameter - this is inherent to
                // std::condition_variable.
                cv.wait_for(lock, calculateWaitDuration(bytes, bytesPerPeriod));
            } else {
                // Not our turn yet, wait to be notified.
                cv.wait(lock);
            }
        }

        // Consume the tokens
        availableTokens -= bytes;

        // Move to next ticket and wake up waiters
        ++servingTicket;
        cv.notify_all();
    }

    /**
     * Get the current number of available tokens (approximate).
     *
     * This is primarily for testing/debugging. The value may be stale
     * by the time the caller uses it.
     *
     * @param bytesPerPeriod The rate in bytes per RateUnit to use for
     *        refilling and capping tokens.
     */
    [[nodiscard]] size_t getAvailableTokens(size_t bytesPerPeriod) {
        std::unique_lock<std::mutex> lock(mutex);
        refillTokens(bytesPerPeriod);
        return availableTokens;
    }

private:
    /**
     * Calculate the duration to wait for sufficient tokens to become available.
     * Must be called with mutex held.
     *
     * @param bytes The number of bytes needed
     * @param bytesPerPeriod The rate in bytes per RateUnit
     * @return Duration to wait
     */
    Duration calculateWaitDuration(size_t bytes, size_t bytesPerPeriod) const {
        if (bytes <= availableTokens) {
            return Duration::zero();
        }
        const size_t tokensNeeded = bytes - availableTokens;

        // time = tokens_needed / bytesPerPeriod * period
        const auto periodNanos =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                        RateUnit{1});
        const auto waitNanos =
                tokensNeeded * periodNanos.count() / bytesPerPeriod;

        return std::chrono::duration_cast<Duration>(
                std::chrono::nanoseconds(waitNanos));
    }

    /**
     * Refill tokens based on elapsed time since last refill.
     * On the first call, initializes the bucket to bytesPerPeriod (full
     * capacity). Must be called with mutex held.
     *
     * @param bytesPerPeriod The rate in bytes per RateUnit
     */
    void refillTokens(size_t bytesPerPeriod) {
        const auto now = Clock::now();
        const auto elapsed = now - lastRefillTime;

        // On first call (lastRefillTime is default-constructed), initialize
        // bucket
        if (lastRefillTime == TimePoint{}) {
            availableTokens = bytesPerPeriod;
            lastRefillTime = now;
            return;
        }

        if (elapsed > Duration::zero()) {
            // Calculate tokens to add based on elapsed time
            // tokens = elapsed_time * bytesPerPeriod / period
            auto elapsedNanos =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                            elapsed);
            auto periodNanos =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                            RateUnit{1});

            size_t tokensToAdd = static_cast<size_t>(elapsedNanos.count()) *
                                 bytesPerPeriod /
                                 static_cast<size_t>(periodNanos.count());

            availableTokens += tokensToAdd;

            // Cap at bucket capacity (= bytesPerPeriod, allowing one period's
            // burst)
            if (availableTokens > bytesPerPeriod) {
                availableTokens = bytesPerPeriod;
            }

            lastRefillTime = now;
        }
    }

    // Current number of available tokens (bytes)
    size_t availableTokens{0};

    // Time of last token refill
    TimePoint lastRefillTime{};

    // Ticket counter for FIFO ordering
    uint64_t nextTicket{0};

    // The ticket currently being served
    uint64_t servingTicket{0};

    // Mutex protecting all mutable state
    std::mutex mutex;

    // Condition variable for blocking/waking threads
    std::condition_variable cv;
};

} // namespace cb
