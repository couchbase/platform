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

#include <platform/platform.h>
#include <condition_variable>
#include <mutex>
#include <string>

namespace Couchbase {

    // forward declaration of the delegator class used to access the private
    // parts of the Thread
    class StartThreadDelegator;

    /**
     * A Thread is a thread used to run a task. It has a mandatory name
     * (will be used as the thread name if the underlying platform supports it)
     */
    class PLATFORM_PUBLIC_API Thread {
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

    protected:
        /**
         * Initialize a new Thread object
         *
         * @param name_ the name of the thread
         */
        Thread(const std::string& name_);

        /**
         * It is not allowed to copy a Thread object
         */
        Thread(const Thread&) = delete;

        /**
         * This is the work the thread should be doing. If you want to be able
         * to stop your thread you need to provide the mechanisms to do so.
         */
        virtual void run() = 0;

    private:
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
         * Is the thread running or not
         */
        bool running;

        /**
         * The thread id for the thread
         */
        cb_thread_t thread_id;
    };
}