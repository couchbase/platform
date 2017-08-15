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

class PipeTest : public ::testing::Test {
protected:
    cb::Pipe buffer;
};

TEST_F(PipeTest, DefaultSize) {
    EXPECT_EQ(2048, buffer.produce([](void*, size_t size) -> ssize_t {
        return size;
    }));
    EXPECT_EQ(2048, buffer.consume([](const void*, size_t size) -> ssize_t {
        return size;
    }));
    EXPECT_TRUE(buffer.empty());
}

TEST_F(PipeTest, EnsureCapacity) {
    buffer.ensureCapacity(100);
    EXPECT_EQ(buffer.capacity(), buffer.wsize());

    size_t capacity = buffer.capacity();
    buffer.produce([capacity](void*, size_t size) -> ssize_t {
        EXPECT_EQ(capacity, size);
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
    EXPECT_EQ(buffer.capacity() - message.size(), buffer.wsize());

    // Read out some of the data
    buffer.consume([](const void*, size_t size) -> ssize_t {
        return 6; // "hello "
    });

    EXPECT_EQ(5, buffer.rsize()); // The buffer should still contain world
    EXPECT_EQ(buffer.capacity() - message.size(), buffer.wsize());

    buffer.ensureCapacity(3000);
    // the buffer should have been doubled in size
    EXPECT_EQ(capacity * 2, buffer.capacity());
    capacity = buffer.capacity();
    buffer.produce([capacity](void*, size_t size) -> ssize_t {
        // the reallocation should be doubled, and the memory in the old
        // buffer was moved to the beginning of the buffer so that we've
        // freed up the data we've already consumed.
        EXPECT_EQ(capacity - 5, size);
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
    const auto capacity = buffer.capacity();
    EXPECT_EQ(3, buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(capacity, size);
        ::memcpy(ptr, "abc", 3);
        return 3;
    }));

    // We should have occupied 3 elements
    buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(capacity - 3, size);
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
    buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(capacity - 3, size);
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
    buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(capacity - 3, size);
        return 0;
    });

    buffer.consume([](const void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(1, size);
        // Let's read out the first (which should be an c)
        EXPECT_EQ('c', *static_cast<const char*>(ptr));
        // But let's not consume it at this time
        return 0;
    });

    // Let's pack the buffer.. that should move the bytes and leave
    // the entire capacity minus one element available in the buffer.
    // pack() returns true if the pipe is empty.
    EXPECT_FALSE(buffer.pack());
    buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        // Everything should have been moved to the front..
        EXPECT_EQ(capacity - 1, size);
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

    // And we should be able to insert the initial capacity number of items
    EXPECT_EQ(capacity, buffer.wsize());
    buffer.produce([capacity](void* ptr, size_t size) -> ssize_t {
        EXPECT_EQ(capacity, size);
        return 0;
    });
}

TEST_F(PipeTest, RellocationSizes) {
    cb::Pipe pipe;

    // Verify that we always double the size of the buffer
    for (int ii = 1; ii < 8; ii++) {
        EXPECT_EQ(2048 << ii, pipe.ensureCapacity(pipe.capacity() + 1));
    }
}
