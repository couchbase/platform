/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <platform/thread.h>
#include <iostream>

class TestThread : public Couchbase::Thread {
public:
    TestThread() : Couchbase::Thread("foo"), tid(0) {}

    std::condition_variable cond;
    std::mutex mutex;
    cb_thread_t tid;

    ~TestThread() override {
        waitForState(Couchbase::ThreadState::Zombie);
    }

protected:
    void run() override {
        setRunning();
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
        GTEST_SKIP();
    }

    EXPECT_EQ(0, cb_set_thread_name("test"));
    char buffer[80];
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));

    EXPECT_EQ("test", cb_get_thread_name(cb_thread_self()));

    memset(buffer, 'a', sizeof(buffer));
    buffer[79] ='\0';
    EXPECT_EQ(1, cb_set_thread_name(buffer)) << " errno " << errno << " "
                                             << strerror(errno);

    memset(buffer, 0, sizeof(buffer));

    // Check that a failing set thread name didn't mess up the value
    EXPECT_EQ(0, cb_get_thread_name(buffer, sizeof(buffer)));
    EXPECT_EQ(std::string("test"), std::string(buffer));
}
