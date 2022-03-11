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

#include <platform/awaitable_semaphore.h>

namespace cb {

void AwaitableSemaphore::release(size_t count) {
    signalWaiters(count);
}

bool AwaitableSemaphore::acquire_or_wait(std::weak_ptr<Waiter> waiter) {
    auto wh = waiters.lock();
    if (try_acquire()) {
        // token was available and has been acquired.
        // If the waiter is already queued for notification, remove it.
        // If we did not, a later release() could notify it, even though
        // it already has a token.
        wh->erase(waiter);
        return true;
    }

    wh->pushUnique(waiter);
    return false;
}

std::vector<std::weak_ptr<Waiter>> AwaitableSemaphore::getWaiters() {
    return waiters.lock()->getWaiters();
}

void AwaitableSemaphore::signalWaiters(size_t count) {
    std::vector<std::shared_ptr<Waiter>> selectedWaiters;

    {
        auto waitersHandle = waiters.lock();
        Semaphore::release(count);
        while (count && !waitersHandle->empty()) {
            auto waiter = waitersHandle->pop().lock();
            if (waiter) {
                selectedWaiters.push_back(waiter);
                count--;
            }
        }
    }

    // signal the waiters outside of the lock.
    // signal could potentially acquire other locks.
    for (auto& waiter : selectedWaiters) {
        waiter->signal();
    }
}

} // namespace cb