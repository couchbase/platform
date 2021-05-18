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

#include <atomic>
#include <gsl/gsl>
#include <stdexcept>

namespace cb {

void Semaphore::release(size_t count) {
    Expects(count != 0);
    tokens.fetch_add(count, std::memory_order_acq_rel);
}

bool Semaphore::try_acquire(size_t count) {
    Expects(count != 0);

    size_t availableTokens = tokens.load(std::memory_order_acquire);
    size_t desired;

    do {
        if (count > availableTokens) {
            return false;
        }

        desired = availableTokens - count;
    } while (!tokens.compare_exchange_weak(
            availableTokens,
            desired,
            std::memory_order_release /* success */,
            std::memory_order_acquire /* failure */));
    return true;
}

} // end namespace cb