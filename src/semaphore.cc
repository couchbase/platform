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