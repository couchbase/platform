/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "platform/non_blocking_mutex.h"

#include <folly/portability/GTest.h>

#include <shared_mutex>

class MockNonBlockingSharedMutex : public cb::NonBlockingSharedMutex {
public:
    using NonBlockingSharedMutex::counter;
    using NonBlockingSharedMutex::SHARED;
    using NonBlockingSharedMutex::UNIQUE;
};

TEST(NonBlockingSharedMutexTest, LockUnlockRepeat) {
    MockNonBlockingSharedMutex mutex;
    EXPECT_EQ(0, mutex.counter);
    {
        std::shared_lock lock(mutex, std::try_to_lock);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_EQ(mutex.SHARED, mutex.counter);
        lock.unlock();
        EXPECT_FALSE(lock.owns_lock());
        EXPECT_EQ(0, mutex.counter);
    }
    EXPECT_EQ(0, mutex.counter);
    {
        std::unique_lock lock(mutex, std::try_to_lock);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_EQ(mutex.UNIQUE, mutex.counter);
        lock.unlock();
        EXPECT_FALSE(lock.owns_lock());
        EXPECT_EQ(0, mutex.counter);
    }
    EXPECT_EQ(0, mutex.counter);
    {
        std::shared_lock lock(mutex, std::try_to_lock);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_EQ(mutex.SHARED, mutex.counter);
    }
    EXPECT_EQ(0, mutex.counter);
    {
        std::unique_lock lock(mutex, std::try_to_lock);
        EXPECT_TRUE(lock.owns_lock());
        EXPECT_EQ(mutex.UNIQUE, mutex.counter);
    }
    EXPECT_EQ(0, mutex.counter);
}

TEST(NonBlockingSharedMutexTest, MultipleShared) {
    MockNonBlockingSharedMutex mutex;
    EXPECT_EQ(0, mutex.counter);
    std::shared_lock shared1(mutex, std::try_to_lock);
    EXPECT_TRUE(shared1.owns_lock());
    EXPECT_EQ(mutex.SHARED, mutex.counter);
    std::shared_lock shared2(mutex, std::try_to_lock);
    EXPECT_TRUE(shared2.owns_lock());
    EXPECT_EQ(mutex.SHARED * 2, mutex.counter);
    std::unique_lock unique1(mutex, std::try_to_lock);
    EXPECT_FALSE(unique1.owns_lock());
    EXPECT_EQ(mutex.SHARED * 2, mutex.counter);
    shared1.unlock();
    EXPECT_EQ(mutex.SHARED, mutex.counter);
    shared2.unlock();
    EXPECT_EQ(0, mutex.counter);
    std::unique_lock unique2(mutex, std::try_to_lock);
    EXPECT_TRUE(unique2.owns_lock());
    EXPECT_EQ(mutex.UNIQUE, mutex.counter);
    std::shared_lock shared3(mutex, std::try_to_lock);
    EXPECT_FALSE(shared3.owns_lock());
    EXPECT_EQ(mutex.UNIQUE, mutex.counter);
}
