/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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
#include <platform/thread.h>

Couchbase::Thread::Thread(const std::string& name_)
    : name(name_),
      running(false) {

}

Couchbase::Thread::~Thread() {
    if (running) {
        cb_join_thread(thread_id);
    }
}

void Couchbase::Thread::thread_entry() {
    cb_set_thread_name(name.c_str());
    running = true;
    // Thread::start grabs the mutex before trying to spawn the thread
    // and will wait on the condition variable so that we can synchronize
    // the startup of the thread.
    {
        std::lock_guard<std::mutex> lock(synchronization.mutex);
    }
    synchronization.cond.notify_one();
    // Ok, now the thread requested this thread to start knows we're
    // up'n'running we can go ahead and call the users function
    run();
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

    if (cb_create_thread(&thread_id, task_thread_main, this, 0) != 0) {
        throw std::bad_alloc();
    }

    synchronization.cond.wait(lock);
}
