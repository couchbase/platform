/*
 *     Copyright 2022-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/semaphore_guard.h>

#include <platform/semaphore.h>

#include <stdexcept>

namespace cb {

template <class SemaphorePtr>
SemaphoreGuard<SemaphorePtr>::SemaphoreGuard(SemaphorePtr semaphorePtr,
                                             size_t tokens)
    : numTokens(tokens) {
    // constructing an "empty" guard by default construction or passing
    // nullptr is allowed.
    if (!semaphorePtr) {
        tokens = 0;
        return;
    }
    // constructing a semaphore guard with a valid semaphore ptr but zero
    // tokens is probably a bug.
    if (!tokens) {
        throw std::invalid_argument(
                "SemaphoreGuard should not track zero tokens");
    }
    if (semaphorePtr->try_acquire(tokens)) {
        // tokens successfully acquired, record the ptr, making this guard
        // valid and now responsible for those tokens.
        this->semaphorePtr = semaphorePtr;
    }
}

template <class SemaphorePtr>
SemaphoreGuard<SemaphorePtr>::SemaphoreGuard(SemaphorePtr semaphorePtr,
                                             adopt_token_t,
                                             size_t tokens)
    : numTokens(tokens) {
    if (!semaphorePtr) {
        tokens = 0;
        return;
    }
    if (semaphorePtr && !tokens) {
        throw std::invalid_argument(
                "SemaphoreGuard should not adopt zero tokens");
    }
    // tokens were already acquired (this constructor is adopting them), record
    // the ptr, making this guard valid and now responsible for those tokens.
    this->semaphorePtr = semaphorePtr;
}

template <class SemaphorePtr>
SemaphoreGuard<SemaphorePtr>::SemaphoreGuard(SemaphoreGuard&& other) {
    *this = std::move(other);
}

template <class SemaphorePtr>
SemaphoreGuard<SemaphorePtr>& SemaphoreGuard<SemaphorePtr>::operator=(
        SemaphoreGuard&& other) {
    reset();

    semaphorePtr = other.semaphorePtr;
    numTokens = other.numTokens;

    other.release();
    return *this;
}

template <class SemaphorePtr>
SemaphoreGuard<SemaphorePtr>::~SemaphoreGuard() {
    reset();
}

template <class SemaphorePtr>
bool SemaphoreGuard<SemaphorePtr>::valid() const {
    return bool(semaphorePtr);
}

template <class SemaphorePtr>
void SemaphoreGuard<SemaphorePtr>::release() {
    semaphorePtr = nullptr;
}

template <class SemaphorePtr>
void SemaphoreGuard<SemaphorePtr>::reset() {
    if (semaphorePtr) {
        semaphorePtr->release(numTokens);
        semaphorePtr = nullptr;
        numTokens = 0;
    }
}

template class SemaphoreGuard<Semaphore*>;
template class SemaphoreGuard<std::shared_ptr<Semaphore>>;
} // namespace cb