/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/semaphore.h>

#include <gsl/gsl-lite.hpp>
#include <atomic>
#include <stdexcept>

namespace cb {

Semaphore::Semaphore(size_t numTokens)
    : capacity(numTokens), tokens(numTokens) {
    if (!capacity) {
        throw std::invalid_argument("Semaphore capacity should be non-zero");
    }
}

void Semaphore::release(size_t count) {
    Expects(count != 0);
    tokens.fetch_add(count, std::memory_order_acq_rel);
}

bool Semaphore::try_acquire(size_t count) {
    Expects(count != 0);

    ssize_t availableTokens = tokens.load(std::memory_order_acquire);
    ssize_t desired;

    do {
        if (ssize_t(count) > availableTokens) {
            return false;
        }

        desired = availableTokens - ssize_t(count);
    } while (!tokens.compare_exchange_weak(
            availableTokens,
            desired,
            std::memory_order_release /* success */,
            std::memory_order_acquire /* failure */));
    return true;
}

void Semaphore::setCapacity(size_t newCapacity) {
    if (!newCapacity) {
        throw std::invalid_argument(
                "Semaphore::setCapacity newCapacity should be greater than "
                "zero");
    }
    auto newValue = newCapacity;
    auto oldValue = capacity.exchange(newValue);
    auto delta = ssize_t(newCapacity) - ssize_t(oldValue);
    if (delta > 0) {
        // capacity has increased, release some "new" tokens
        release(delta);
    } else if (delta < 0) {
        // capacity has decreased, throw away some tokens
        // this might make tokens temporarily negative, but that's
        // okay. Once all outstanding users release, tokens == newCapacity
        tokens.fetch_add(delta);
    }
}

} // end namespace cb