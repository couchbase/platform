/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <atomic>
#include <cstdint>

namespace cb {

/**
 * A lightweight alternative to std::shared_mutex
 *
 * Only supports try_lock*, without failing spuriously.
 * Use with std::unique_lock or std::shared_lock, with std::try_to_lock.
 */
class NonBlockingSharedMutex {
public:
    NonBlockingSharedMutex() = default;

    NonBlockingSharedMutex(const NonBlockingSharedMutex&) = delete;

    NonBlockingSharedMutex(NonBlockingSharedMutex&&) = delete;

    /**
     * Not to be used directly. Use std::unique_lock instead.
     * Tries to acquire a unique lock, which succeeds if no lock
     * (unique or shared) is held.
     * @return true if locking succeeded
     */
    bool try_lock() {
        // Set the lowest bit of the counter, provided that the current value
        // of the counter is 0. If setting succeeds, the lock is now held.
        // On success, memory_order_acquire is used so that writes from other
        // threads that released the counter are visible in this thread.
        // Prior writes of this thread don't need to be made visible to other
        // threads yet.
        // On failure, we don't enter the critical section, so we can relax
        // the memory order.

        // The value the counter must have for it to be set to THRESHOLD.
        // Note that it will be replaced by the actual value, which we ignore.
        std::uint32_t expected = 0;
        // compare_exchange_weak is allowed to fail spuriously.
        return counter.compare_exchange_strong(expected,
                                               UNIQUE,
                                               std::memory_order_acquire,
                                               std::memory_order_relaxed);
    }

    /**
     * Not to be used directly. Use std::unique_lock instead.
     * Releases a previously held unique lock.
     * Calling without holding a unique lock will result in invalid state.
     */
    void unlock() {
        // Clear the lowest bit of the counter. Subtraction is used as
        // try_lock_shared() may have incremented the counter.
        // memory_order_release is used as prior writes of this thread need to
        // be made visible to other threads when exiting the critical section.
        // Writes from other threads don't need to be made visible to this
        // thread.
        counter.fetch_sub(UNIQUE, std::memory_order_release);
    }

    /**
     * Not to be used directly. Use std::shared_lock instead.
     * Tries to acquire a shared lock, which succeeds if no unique lock is held,
     * even if other shared locks are held.
     * @return true if locking succeeded
     */
    bool try_lock_shared() {
        // Increment the counter to keep track of shared locks.
        // memory_order_acquire is used as we are about to enter the critical
        // section and writes from other threads that held a unique lock need
        // to be made visible in this thread.
        if (counter.fetch_add(SHARED, std::memory_order_acquire) & UNIQUE) {
            // The lowest bit of the counter is set, so a unique lock is held.
            // We don't enter the critical section and need to undo the counter
            // increment.
            counter.fetch_sub(SHARED, std::memory_order_relaxed);
            return false;
        }
        return true;
    }

    /**
     * Not to be used directly. Use std::shared_lock instead.
     * Releases a previously held shared lock.
     * Calling without holding a shared lock will result in invalid state.
     */
    void unlock_shared() {
        // Decrement the counter to remove one shared lock.
        // memory_order_release is used as prior (atomic) writes of this thread
        // may need to be ordered before lock release to other threads.
        // Writes from other threads don't need to be made visible to this
        // thread.
        counter.fetch_sub(SHARED, std::memory_order_release);
    }

protected:
    static constexpr std::uint32_t UNIQUE = 1;
    static constexpr std::uint32_t SHARED = 2;

    // Lowest bit set when unique lock is held.
    // The remaining bits count the shared locks.
    std::atomic<std::uint32_t> counter{0};
};

} // namespace cb
