/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/system/ThreadName.h>
#include <phosphor/phosphor.h>
#include <platform/thread.h>
#include <utility>

Couchbase::Thread::Thread(std::string name_)
    : name(std::move(name_)), state(ThreadState::Stopped) {
}

Couchbase::Thread::~Thread() {
    switch (state.load()) {
    case ThreadState::Stopped:
        return;
    case ThreadState::Zombie:
        thread.join();
        return;
    case ThreadState::Running:
    case ThreadState::Starting:
        // Both of these states are invalid (should not destruct if Running or
        // Starting), however destructors cannot fail so should not throw
        // or similar here. *If* we had access to a logger we might want to log
        // something...
        break;
    }
}

void Couchbase::Thread::thread_entry() {

    // Call the subclass run() method
    run();


    if (state != ThreadState::Running) {
        throw std::logic_error("Couchbase::Thread::thread_entry Subclass should"
                                   " call setRunning()");
    }

    setState(ThreadState::Zombie);
}

class Couchbase::StartThreadDelegator {
public:
    static void run(Thread& thread) {
        thread.thread_entry();
    }
};

static void task_thread_main(void* arg) {
    auto* thread = reinterpret_cast<Couchbase::Thread*>(arg);
    Couchbase::StartThreadDelegator::run(*thread);
}

void Couchbase::Thread::start() {
    std::unique_lock<std::mutex> lock(synchronization.mutex);
    state = ThreadState::Starting;

    thread = std::thread{[this]() { task_thread_main(this); }};

    while (state != ThreadState::Running && state != ThreadState::Zombie) {
        synchronization.cond.wait(lock);
    }
}

void Couchbase::Thread::setRunning() {
    setState(ThreadState::Running);
}

Couchbase::ThreadState Couchbase::Thread::waitForState(
    const Couchbase::ThreadState& newState) {

    std::unique_lock<std::mutex> lock(synchronization.mutex);

    while (true) {
        ThreadState current = getState();

        if (newState == current || current == ThreadState::Stopped) {
            // We're in the current state, or the thread is at the final state
            return current;
        }

        if (static_cast<int>(newState) < static_cast<int>(current)) {
            // We can never reach this state.
            return current;
        }
        synchronization.cond.wait(lock);
    }
}

bool cb_set_thread_name(std::string_view name) {
    if (name.size() > MaxThreadNameLength) {
        throw std::logic_error("cb_set_thread_name: thread name too long");
    }

    return folly::setThreadName(name);
}

std::thread create_thread(std::function<void()> main, std::string name) {
    // We don't want the exception being thrown in the new thread...
    if (name.size() > MaxThreadNameLength) {
        throw std::logic_error("create_thread: thread name too long");
    }

    return std::thread{[n = std::move(name), main]() {
        if (folly::canSetCurrentThreadName()) {
            folly::setThreadName(n);
        }
        PHOSPHOR_INSTANCE.registerThread(n);
        main();
        PHOSPHOR_INSTANCE.deregisterThread();
    }};
}
