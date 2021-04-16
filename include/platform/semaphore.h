/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2021 Couchbase, Inc
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