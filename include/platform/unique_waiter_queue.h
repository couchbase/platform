/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include "semaphore_guard.h"

#include <deque>
#include <memory>
#include <set>
#include <vector>

namespace cb {

class AwaitableSemaphore;
class Waiter;

/**
 * Queue of cb::Waiters which ensures queued waiters are unique.
 *
 * This allows the equivalent of "spurious wakeups" - if a Waiter (e.g., a task)
 * can be triggered to run by something other than the semaphore, it must be
 * safe for it to wait on the semaphore again.
 *
 * If waiters were not unique the same waiter could be signalled twice, instead
 * of two distinct waiters, leaving tokens available and waiters sleeping.
 *
 * See std::owner_less for how weak ptrs are compared.
 */
class UniqueWaiterQueue {
public:
    using WaiterPtr = std::weak_ptr<Waiter>;

    UniqueWaiterQueue() = default;

    // disallow copying - queue holds iterators into waiterSet which
    // would still point into the copied-from waiterSet.
    UniqueWaiterQueue(const UniqueWaiterQueue&) = delete;
    UniqueWaiterQueue& operator=(const UniqueWaiterQueue&) = delete;

    // Allow move
    UniqueWaiterQueue(UniqueWaiterQueue&& other) = default;
    UniqueWaiterQueue& operator=(UniqueWaiterQueue&& other) = default;

    /**
     * Try to add a waiter to the queue.
     *
     * If the waiter is already in the queue, do nothing.
     */
    void pushUnique(WaiterPtr waiter);

    /**
     * Try to erase a waiter from the queue.
     *
     * If the waiter is not in the queue, do nothing.
     */
    void erase(const WaiterPtr& waiter);

    /**
     * Pop a waiter from the front of the queue.
     *
     * If empty, returns a nullptr.
     */
    WaiterPtr pop();

    bool empty() const;

    /**
     * Get the current tasks waiting in this queue.
     *
     * Test-only.
     */
    std::vector<WaiterPtr> getWaiters() const;

private:
    using WaiterSet = std::set<WaiterPtr, std::owner_less<WaiterPtr>>;
    // set of weak pointers to Waiters which are currently waiting for
    // the owning semaphore
    WaiterSet waiterSet;
    // queue of iterators pointing into the waiterSet, tracks the order
    // waiters were queued in.
    // Deque (rather than queue) to allow erasing.
    std::deque<WaiterSet::iterator> queue;
};

/**
 * Interface to be implemented by types wishing to wait for tokens to become
 * available in an AwaitableSemaphore.
 *
 * Once N tokens are released to the semaphore, at most N waiters will be
 * signal()-ed. The waiter _must_ then try to acquire a token again ("soon", not
 * necessarily in signal()); if it does not try to acquire a token, other
 * waiters will not be notified, despite tokens now being available.
 *
 * It is not guaranteed that a token will be available after signal(); the
 * waiter should be prepared to wait repeatedly if other actors acquire the
 * token.
 *
 */
class Waiter : public std::enable_shared_from_this<Waiter> {
public:
    /**
     * Callback to inform the waiter that a token may now be available, and
     * it should try to aquire one again.
     *
     * Should not do "heavy" work, but should be used to e.g., wake a snoozed
     * task.
     */
    virtual void signal() = 0;

    virtual ~Waiter();
};

} // namespace cb
