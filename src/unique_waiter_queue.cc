/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/unique_waiter_queue.h>

#include <platform/awaitable_semaphore.h>

namespace cb {

void UniqueWaiterQueue::pushUnique(UniqueWaiterQueue::WaiterPtr waiter) {
    // try to insert into the set
    auto [itr, inserted] = waiterSet.insert(waiter);
    if (inserted) {
        // the waiter was not already in the set, queue
        queue.push_back(itr);
    }
}

void UniqueWaiterQueue::erase(const UniqueWaiterQueue::WaiterPtr& waiter) {
    // try to find the waiter the set
    auto itr = waiterSet.find(waiter);
    if (itr == waiterSet.end()) {
        // not in the queue, nothing to do
        return;
    }

    // remove it from the ordered queue
    queue.erase(std::remove(queue.begin(), queue.end(), itr), queue.end());
    // and from the set
    waiterSet.erase(itr);
}

UniqueWaiterQueue::WaiterPtr UniqueWaiterQueue::pop() {
    if (empty()) {
        return {};
    }
    // find the next waiter
    auto waiterItr = queue.front();
    // remove it from the queue
    queue.pop_front();
    auto waiter = *waiterItr;
    // remove it from the set
    waiterSet.erase(waiterItr);

    return waiter;
}
bool UniqueWaiterQueue::empty() const {
    return queue.empty();
}

std::vector<UniqueWaiterQueue::WaiterPtr> UniqueWaiterQueue::getWaiters()
        const {
    return {waiterSet.begin(), waiterSet.end()};
}

Waiter::~Waiter() = default;

} // namespace cb
