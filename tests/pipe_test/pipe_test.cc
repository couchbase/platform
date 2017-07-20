/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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

#include <platform/pipe.h>

#include <gtest/gtest.h>

class MockPipe : public cb::Pipe {
public:
    size_t capacity() const {
        return buffer.size();
    }
};

class PipeTest : public ::testing::Test {
protected:
    MockPipe buffer;
};

TEST_F(PipeTest, DefaultSize) {
    EXPECT_EQ(0, buffer.produce([](void*, size_t size) -> ssize_t {
        return size;
    }));
    EXPECT_EQ(0, buffer.consume([](const void*, size_t size) -> ssize_t {
        return size;
    }));
    EXPECT_TRUE(buffer.empty());
}

TEST_F(PipeTest, Locking) {
    buffer.lock();
    EXPECT_TRUE(buffer.empty());

    // but not change it
    EXPECT_THROW(buffer.clear(), std::logic_error);
    EXPECT_THROW(buffer.pack(), std::logic_error);
    EXPECT_THROW(buffer.consume([](const void*, size_t size) -> ssize_t {
        return size;
    }),
                 std::logic_error);
    EXPECT_THROW(
            buffer.produce([](void*, size_t size) -> ssize_t { return size; }),
            std::logic_error);
    EXPECT_THROW(buffer.ensureCapacity(1), std::logic_error);

    // And not lock a locked buffer
    EXPECT_THROW(buffer.lock(), std::logic_error);

    buffer.unlock();

    // but we can't unlock twice
    EXPECT_THROW(buffer.unlock(), std::logic_error);
}

TEST_F(PipeTest, EnsureCapacity) {
    buffer.ensureCapacity(100);
    EXPECT_EQ(100, buffer.wsize());
    buffer.produce([](void*, size_t size) -> ssize_t {
        EXPECT_EQ(100, size);
        return 0;
    });

    buffer.consume([](const void*, size_t size) -> ssize_t {
        // It should be empty..
        EXPECT_EQ(0, size);
        return 0;
    });
    EXPECT_EQ(0, buffer.rsize());

    // Make sure that it keep the data between the iterations, even if it isn't
    // at the beginning of the buffer
    const std::string message{"hello world"};
    buffer.produce([&message](cb::byte_buffer buffer) -> ssize_t {
        std::copy(message.begin(), message.end(), buffer.data());
        return message.size();
    });

    EXPECT_EQ(message.size(), buffer.rsize());
    EXPECT_EQ(100 - message.size(), buffer.wsize());

    // Read out some of the data
    buffer.consume([](const void*, size_t size) -> ssize_t {
        return 6; // "hello "
    });

    buffer.ensureCapacity(1024);
    buffer.produce([](void*, size_t size) -> ssize_t {
        // The buffer should at least have room for 100 elements
        EXPECT_LE(1024, size);
        return 0;
    });

    // Make sure that the data was persisted across the move
    buffer.consume([](cb::const_byte_buffer buffer) -> ssize_t {
        const std::string data{reinterpret_cast<const char*>(buffer.data()),
                               buffer.size()};
        EXPECT_EQ("world", data);
        return data.length();
    });
}

TEST_F(PipeTest, ProduceOverfow) {
    buffer.ensureCapacity(100);
    EXPECT_THROW(buffer.produce([](void*, size_t size) -> ssize_t {
        return size + 1;
    }),
                 std::logic_error);
}

TEST_F(PipeTest, ConsumeOverfow) {
    buffer.ensureCapacity(100);
    EXPECT_THROW(buffer.consume([](const void*, size_t size) -> ssize_t {
        return size + 1;
    }),
                 std::logic_error);
}

TEST_F(PipeTest, ProduceConsume) {
    buffer.ensureCapacity(100);
    EXPECT_EQ(3, buffer.produce([](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(100, size);
        ::memcpy(ptr, "abc", 3);
        return 3;
    }));

    // We should have 97 elements available
    buffer.produce([](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(97, size);
        return 0;
    });

    // And 3 available..
    buffer.consume([](const void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(3, size);

        // Let's read out the first (which should be an a)
        EXPECT_EQ('a', *static_cast<const char*>(ptr));
        return 1;
    });

    // We've consumed only 1 byte so the produce space is unchanged,
    // but the consumer space is less
    buffer.produce([](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(97, size);
        return 0;
    });

    buffer.consume([](const void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(2, size);
        // Let's read out the first (which should be an b)
        EXPECT_EQ('b', *static_cast<const char*>(ptr));
        return 1;
    });

    // We've consumed only 1 byte so the produce space is unchanged,
    // but the consumer space is less
    buffer.produce([](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(97, size);
        return 0;
    });

    buffer.consume([](const void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(1, size);
        // Let's read out the first (which should be an c)
        EXPECT_EQ('c', *static_cast<const char*>(ptr));
        // But let's not consume it at this time
        return 0;
    });

    // Let's pack the buffer.. that should move the bytes and leave 99
    // buffers available in the buffer (pack returns true if the
    // pipe is empty)
    EXPECT_FALSE(buffer.pack());
    buffer.produce([](void* ptr, size_t size) -> ssize_t {
        // Everything should have been moved to the front..
        EXPECT_EQ(99, size);
        return 0;
    });

    buffer.consume([](const void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(1, size);
        // Let's read out the first (which should be an c)
        EXPECT_EQ('c', *static_cast<const char*>(ptr));
        return 1;
    });

    // The pipe should be empty now..
    EXPECT_TRUE(buffer.empty());

    // And trying to pack it should return true
    EXPECT_TRUE(buffer.pack());

    // And we should be able to insert 100 items again
    buffer.produce([](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(100, size);
        return 0;
    });
}
