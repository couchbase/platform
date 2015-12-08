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
#include <gtest/gtest.h>
#include <platform/thread.h>
#include <iostream>

class TestThread : public Couchbase::Thread {
public:
    TestThread() : Couchbase::Thread("foo"), tid(0) {}

    std::condition_variable cond;
    std::mutex mutex;
    cb_thread_t tid;

protected:
    virtual void run() override {
        std::lock_guard<std::mutex> guard(mutex);
        tid = cb_thread_self();
        cond.notify_all();
    }
};

class ThreadTest : public ::testing::Test {
};


TEST_F(ThreadTest, SimpleThreadTest) {
    TestThread worker;
    std::unique_lock<std::mutex> lock(worker.mutex);
    EXPECT_NO_THROW(worker.start());
    worker.cond.wait(lock);

    EXPECT_NE((cb_thread_t)0, worker.tid);
    EXPECT_NE(cb_thread_self(), worker.tid);
}

TEST(ThreadnameTest, ThreadName) {
    if (!is_thread_name_supported()) {
        return;
    }

    EXPECT_EQ(0, cb_set_thread_name("test"));
    char buffer[80];
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));

    memset(buffer, 'a', sizeof(buffer));
    buffer[79] ='\0';
    EXPECT_EQ(1, cb_set_thread_name(buffer)) << " errno " << errno << " "
                                             << strerror(errno);

    memset(buffer, 0, sizeof(buffer));

    // Check that a failing set thread name didn't mess up the value
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));
}
