#include <platform/unique_waiter_queue.h>

#include <platform/awaitable_semaphore.h>

namespace cb {

void UniqueWaiterQueue::pushUnique(UniqueWaiterQueue::WaiterPtr waiter) {
    // try to insert into the set
    auto [itr, inserted] = waiterSet.insert(waiter);
    if (inserted) {
        // the waiter was not already in the set, queue
        queue.push(itr);
    }
}

UniqueWaiterQueue::WaiterPtr UniqueWaiterQueue::pop() {
    if (empty()) {
        return {};
    }
    // find the next waiter
    auto waiterItr = queue.front();
    // remove it from the queue
    queue.pop();
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
