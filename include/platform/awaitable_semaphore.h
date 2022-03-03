/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#include "semaphore.h"

#include "unique_waiter_queue.h"

#include <folly/Synchronized.h>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

namespace cb {

/**
 * Semaphore variant tracking a queue of actors waiting to acquire a token.
 *
 * Useful if users do not wish to block to wait for a resource (e.g.,
 * GlobalTask) but wish to be notified once a token becomes available.
 *
 * Example usage:
 *
 * FooBarTask::run() {
 *     if (!semaphore.acquire_or_wait(shared_from_this())) {
 *         // snooze() forever.
 *         return true;
 *     }
 *     // token was acquired, do some semaphore-protected work
 *     semaphore.release();
 * }
 *
 * FooBarTask::signal() {
 *     // wake the task, so that it calls run() again, and tries to acquire
 *     // a token again.
 * }
 *
 */
class AwaitableSemaphore : public Semaphore {
public:
    // pull in constructors
    using Semaphore::Semaphore;

    using Semaphore::release;

    /**
     * Return @p count tokens to the semaphore.
     *
     * If there are queued waiters, they will be signalled to run again.
     */
    void release(size_t count) override;

    /**
     * Attempt to acquire a token from the semaphore, or queue for notification
     * if no tokens are available.
     *
     * @param waiter waiter which will be queued if a token cannot be acquired
     * @return true if a token was acquired, else false.
     */
    bool acquire_or_wait(std::weak_ptr<Waiter> waiter);

    /**
     * Get the current tasks waiting for this semaphore.
     *
     * Test-only.
     */
    std::vector<std::weak_ptr<Waiter>> getWaiters();

protected:
    void signalWaiters(size_t count);

    folly::Synchronized<UniqueWaiterQueue, std::mutex> waiters;
};

} // namespace cb