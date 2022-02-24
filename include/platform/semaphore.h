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

#include <sys/types.h>

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
    explicit Semaphore(size_t numTokens);

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

    /**
     * Change the maximum number of tokens available from this semaphore.
     *
     * If increased, "new" tokens become available.
     *
     * If decreased, tokens are logically removed, but there may be more
     * existing token holders than newCapacity. In this case, `tokens`
     * may temporarily become negative.
     * In that case, no additional tokens can be acquired until enough existing
     * holders call release() such that `tokens` is increased above zero.
     *
     * Once all tokens are released, tokens=newCapacity.
     *
     * @param newCapacity the new total number of tokens
     */
    void setCapacity(size_t newCapacity);

    size_t getCapacity() const {
        return capacity.load();
    }

private:
    // maximum number of tokens which can be acquired from this semaphore
    // before further try_acquire() calls fail.
    std::atomic<size_t> capacity;
    // Current number of available tokens. As callers acquire tokens this will
    // decrease, and increase on release().
    // try_acquire() will fail if decrementing tokens would lead to a < 0 value.
    // However, this is signed because setCapacity() may decrease the capacity
    // below the number of tokens currently acquired. In that case, `tokens`
    // _will_ temporarily become negative, until enough of those tokens are
    // released().
    std::atomic<ssize_t> tokens;
};
} // end namespace cb