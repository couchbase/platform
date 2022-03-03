/*
 *     Copyright 2022-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <memory>

namespace cb {
class Semaphore;

// tag used when constructing SempahoreGuard indicating caller already holds
// a token. (similar to std::adopt_lock_t).
struct adopt_token_t {};

static constexpr adopt_token_t adopt_token;

/**
 * RAII guard which represents a number of tokens which have been acquired
 * from a Semaphore, and must be returned to the semaphore when the guard
 * is destroyed.
 * @tparam SemaphorePtr
 */
template <class SemaphorePtr = Semaphore*>
class SemaphoreGuard {
public:
    /**
     * Construct a guard. Attempts to acquire `tokens` tokens from the
     * provided semaphore.
     *
     * If it succeeds, the created guard is valid and becomes responsible for
     * the tokens, and will release them on destruction.
     * Else, the guard is invalid, and will do nothing on destruction.
     *
     * Caller should check
     *   guard.valid();
     *  or
     *  if (guard) {...}
     *
     */
    SemaphoreGuard(SemaphorePtr semaphorePtr = nullptr, size_t tokens = 1);

    /**
     * Construct a guard taking responsibility for a token which has already
     * been acquired by the caller.
     *
     * Will not acquire additional tokens, but will release `tokens` tokens
     * on destruction.
     */
    SemaphoreGuard(SemaphorePtr semaphorePtr, adopt_token_t, size_t tokens = 1);

    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard(SemaphoreGuard&&);

    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(SemaphoreGuard&&);

    ~SemaphoreGuard();

    /**
     * Check if this guard manages any tokens.
     *
     * If true, the guard is tracking one or more tokens, which will be
     * returned to a semaphore on destruction or reset.
     * If false, no tokens are managed. The guard may still be used e.g.,
     * it may be moved into, and release()/reset() are no-ops.
     * @return
     */
    bool valid() const;

    operator bool() const {
        return valid();
    }

    /**
     * Stop tracking the contained tokens.
     *
     * The tokens will not be returned to the semaphore on destruction,
     * instead the caller is now responsible for that.
     *
     * If this guard does not manage any tokens ( i.e., is !valid() ), this
     * is a no-op.
     */
    void release();

    /**
     * Immediately return any managed tokens to the semahpore.
     *
     * The tokens are returned to the Semaphore immediately, and will no longer
     * be returned at destruction of the guard.
     *
     * If this guard does not manage any tokens ( i.e., is !valid() ), this
     * is a no-op.
     */
    void reset();

    size_t getNumTokens() const {
        return numTokens;
    }

private:
    /**
     * The number of tokens this guard currently manages. This number of
     * tokens will be returned to the semaphore on destruction/reset.
     */
    size_t numTokens = 0;

    /**
     * Pointer to the semaphore to which tokens will be returned on destruction
     * or reset(). If null, this guard is !valid() and tracks no tokens.
     */
    SemaphorePtr semaphorePtr = nullptr;
};

extern template class SemaphoreGuard<Semaphore*>;
extern template class SemaphoreGuard<std::shared_ptr<Semaphore>>;
} // namespace cb