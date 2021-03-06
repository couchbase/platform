/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <atomic>
#include <cstddef>


namespace cb {

/**
 * Simple Semaphore with no support for blocking.
 *
 * Exposes release() and try_acquire(), with similar semantics to
 * std::counting_semaphore.
 * std::counting_semaphore:
 *  * is not available until C++20
 *
 * Folly offers BatchSemaphore which:
 *  * is available
 *  * takes a runtime token count
 *  * expects caller to block (potentially taking a Waiter&, which then needs to
 *    remain owned by the caller until the Semaphore is signalled).
 *  * pulls in a number of additional headers.
 *
 * On balance, this implementation is simple and light-weight, and provides the
 * bare minimum required for its intended use case.
 *
 * [1]: https://en.cppreference.com/w/cpp/thread/counting_semaphore
 */
class Semaphore {
public:
    Semaphore() : Semaphore(1) {
    }
    explicit Semaphore(size_t numTokens)
        : capacity(numTokens), tokens(numTokens) {
    }

    Semaphore(const Semaphore&) = delete;
    Semaphore(Semaphore&&) = delete;

    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore& operator=(Semaphore&&) = delete;

    /**
     * Return `count` tokens to the semaphore.
     */
    void release(size_t count = 1);

    /**
     * If the semaphore has an available token, decrement the available tokens
     * and return true.
     *
     * Note: there is no similar `acquire()` method - this semaphore does not
     * support blocking.
     *
     * @param count how many tokens to attempt to acquire
     * @return true if tokens were available and have now been acquired, else
     * false
     */
    bool try_acquire(size_t count = 1);

    size_t getCapacity() const {
        return capacity;
    }

private:
    const size_t capacity;
    std::atomic<size_t> tokens;
};
} // end namespace cb