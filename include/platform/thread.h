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
#pragma once

#include <platform/platform_thread.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

namespace Couchbase {

    // forward declaration of the delegator class used to access the private
    // parts of the Thread
    class StartThreadDelegator;

    /**
     * The various states the Thread Object may be in.
     *
     * As a client of the library you should <b>not</b> depend on the
     * enum values meaning anything.
     *
     * Do <b>NOT</b> change the values for each state, the internals of
     * the Thread class depend on internal order between the values.
     */
    enum class ThreadState {
        /** The thread is not running (and have never been started) */
        Stopped = 0,
        /**
         * The thread is starting, but the threads metod run() have not
         * called setRunning() yet (the start() method has not yet returned)
         */
        Starting = 1,
        /**
         * The thread is running in the subclass run() method
         */
        Running = 2,
        /**
         * The subclass run() method returned, and the thread is stopped
         * (but not reaped yet by joining the return code from the thread)
         */
        Zombie = 3
    };

    /**
     * A Thread is a thread used to run a task. It has a mandatory name
     * (will be used as the thread name if the underlying platform supports it)
     */
    class Thread {
    public:
        /**
         * Release all resources allocated by the Thread (if the thread is
         * running we'll wait for it to complete...)
         */
        virtual ~Thread();

        /**
         * Request to start the Thread
         *
         * The start method will try to spawn the thread object and <b>block</b>
         * until the thread is running.
         *
         * @throws std::bad_alloc if we're failing to spawn a new thread
         */
        void start();

        /**
         * Get the current state of the thread
         */
        [[nodiscard]] ThreadState getState() const {
            return state.load();
        }

        /**
         * Wait for the thread to enter a certain state.
         *
         * The wait is terminated if the thread enters a state which would
         * cause it to never reach the requested state (so you have to check
         * the state when the method returns)
         *
         * @param state the requested state
         * @return the state we ended up in
         */
        ThreadState waitForState(const ThreadState& state);

    protected:
        /**
         * Initialize a new Thread object
         *
         * @param name_ the name of the thread
         */
        Thread(std::string name_);

        /**
         * It is not allowed to copy a Thread object
         */
        Thread(const Thread&) = delete;

        /**
         * This is the work the thread should be doing. If you want to be able
         * to stop your thread you need to provide the mechanisms to do so.
         *
         * In your subclass you must start by calling setRunning() so that
         * the client users of your subclass can utilize your class.
         * Failing to do so will cause start() to block until the run
         * method completes.
         *
         * ex:
         *     void MyThread::run() override {
         *         ... my local initialization ..
         *
         *         // Unblock the Thread::start() method.
         *         setRunning();
         *
         *         ... my thread code ...
         *     }
         */
        virtual void run() = 0;

        /**
         * The subclass needs to call setRunning as the first method inside
         * it's run method so that the thread object knows that the thread
         * is indeed running
         */
        void setRunning();

    private:

        void setState(const ThreadState& st) {
            std::lock_guard<std::mutex> lock(synchronization.mutex);
            state = st;
            synchronization.cond.notify_all();
        }

        friend class StartThreadDelegator;

        /**
         * In order to synchronize the start of the thread we'll just use a
         * condition variable to block the caller until the thread is running
         */
        struct {
            std::condition_variable cond;
            std::mutex mutex;
        } synchronization;

        /**
         * The entry point for the thread where global thread initialization is
         * performed (setting thread name, signalling the thread creator etc)
         */
        void thread_entry();

        /**
         * The name of the thread
         */
        std::string name;

        /**
         * The thread id for the thread
         */
        cb_thread_t thread_id;

        /**
         * The state of the thread
         */
        std::atomic<ThreadState> state;
    };
}
