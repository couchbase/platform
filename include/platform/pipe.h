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
#pragma once

#include <platform/cb_malloc.h>
#include <platform/platform.h>
#include <platform/sized_buffer.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace cb {

/**
 * The Pipe class may integrate with the buffer logic in valgrind to trap
 * incorrect buffer usage if CB_PIPE_VALGRIND_INTEGRATION is defined
 * for the project. Given that we don't have any numbers on the impact
 * enabling/disabling read/write access to memory areas have on the overall
 * system, we'll leave it as disabled for now.
 */
#undef CB_PIPE_VALGRIND_INTEGRATION

/**
 * The Pipe class is a buffered pipe where you may insert data in one
 * end, and read it back out from the other (like a normal pipe).
 *
 * Instead of using customized iosteram buffers and allows for that kind
 * of use, this class provides a "produce" method and a "consume" method
 * where you provide a callback which receives the memory area it may operate
 * on so that it can perform more optimal IO by populating the buffer
 * directly:
 *
 *    pipe.produce([&sock](void* data, size_t size) -> ssize_t {
 *        return cb::net::recv(sock, data, size, 0);
 *    });
 *
 *    And now you may use the data by calling:
 *
 *    pipe.consume([](const void* data, size_t size) -> ssize_t {
 *        // do whatever we want with the data
 *        return nbytes;
 *    });
 *
 * The return value from the consume method is the number of bytes you've
 * consumed (so that they may be dropped from the pipe). It means that if
 * you just want to look at the bytes you would return 0, and they would
 * still be present the next time you invoke the consume method.
 *
 * The pipe class is _not_ thread-safe as it provides absolutely no
 * locking internally.
 *
 * Implementation details:
 *
 * The Pipe is implemented by using allocating an underlying buffer of a
 * fixed size. The caller may grow this buffer by calling ensureCapacity,
 * which would:
 *
 *    * return immediately if available segment at the end is big enough
 *    * pack the buffer if the free segment at the end and the beginning
 *      is big enough
 *    * reallocate the underlying buffer if we need more space
 *
 * As you can see from the above the last two bullets would invalidate
 * all addresses previously provided to the "produce" and "consume" methods
 * which means that you _cannot_ keep pointers into this buffer and use
 * ensureCapacity.
 *
 * Data written to the pipe is always put at the end of the allocated
 * buffer, and the write_head is moved forward. Data is always read from
 * the read_head, and once consumed the read head is moved forward. Whenever
 * the read_head catch up with the write_head they're both set to 0 (the
 * beginning of the buffer).
 */
class PLATFORM_PUBLIC_API Pipe {
public:
    /**
     * Initialize a pipe with the given buffer size (default empty).
     *
     * The write end of the pipe may be increased by calling ensureCapacity.
     *
     * @param size The initial size of the buffer in the pipe (default 0)
     */
    explicit Pipe(size_t size = 0);

    /**
     * Make sure that one may insert at least the specified number
     * of bytes in the buffer.
     *
     * This method might reallocate the buffers and invalidate all pointers
     * into the buffer.
     *
     * @param nbytes The number of bytes needed in the buffer
     * @return The number of bytes of available space in the buffer (for the
     *         write end)
     * @throws std::bad_alloc if memory allocation fails
     */
    size_t ensureCapacity(size_t nbytes);

    /**
     * Read the number of bytes currently available in the read end
     * of the pipe
     */
    size_t rsize() const {
        return getAvailableReadSpace().size();
    }

    /**
     * Get the available read buffer (this may be used for simplicity
     * rather than calling consume to have to copy it out)
     */
    cb::const_byte_buffer rdata() const {
        return getAvailableReadSpace();
    }

    /**
     * Returns the number of bytes available to be written to in the
     * write end of the pipe
     */
    size_t wsize() const {
        return getAvailableWriteSpace().size();
    }

    /**
     * Try to produce a number of bytes by providing a callback function
     * which will receive the buffer where the data may be inserted
     *
     * @param producer a callback function to produce data into the
     *                 continuous memory area from ptr and size bytes
     *                 long
     * @return the number of bytes produced
     */
    ssize_t produce(std::function<ssize_t(void* /* ptr */, size_t /* size */)>
                            producer);

    /**
     * Try to produce a number of bytes by providing a callback function
     * which will receive the buffer where the data may be inserted
     *
     * @param producer a callback function to produce data into the
     *                 provided buffer.
     * @return the number of bytes produced
     */
    ssize_t produce(std::function<ssize_t(cb::byte_buffer)> producer);

    /**
     * Try to consume data from the buffer by providing a callback function
     *
     * @param producer a callback function to consume data from the provided
     *                 continuous memory area from ptr and size bytes long.
     *                 The number of bytes consumed should be returned.
     * @return the number of bytes consumed
     */
    ssize_t consume(std::function<ssize_t(const void* /* ptr */,
                                          size_t /* size */)> consumer);

    /**
     * Try to consume data from the buffer by providing a callback function
     *
     * @param producer a callback function to consume data from the provided
     *                 memory area. The number of bytes consumed should be
     *                 returned.
     * @return the number of bytes consumed
     */
    ssize_t consume(std::function<ssize_t(cb::const_byte_buffer)> consumer);

    /**
     * Pack this buffer
     *
     * Given that we use a write_head and read_head in an array instead of
     * using a ringbuffer one may end up in the situation where the pipe
     * contains a single byte, but we can't insert any data because that
     * byte is at the end of the pipe.
     *
     * Packing the buffer moves all of the bytes in the pipe to the beginning
     * of the internal buffer resulting in a larger available memory segment
     * at the end.
     *
     * @return true if the buffer is empty after packing
     */
    bool pack();

    /**
     * Is this buffer empty (the consumer end completely caught up with
     * the producer)
     */
    bool empty() const;

    /**
     * Is this buffer full or not
     */
    bool full() const;

    /**
     * Clear all of the content in the buffer
     */
    void clear();

    /**
     * Mark this buffer as "locked". When the buffer is "locked" any
     * attempt that would change the state of the buffer would throw
     * an exception.
     *
     * The primary use for this is for testing (and sanity checking) to
     * ensure that we don't mess around with pipe when we don't expect
     * clients to use them (In the memcached core we may move empty
     * pipes between threads).
     */
    void lock();

    /**
     * Release the locked state
     */
    void unlock();

    /**
     * Get the (internal) properties of the pipe
     *
     * This allows other tools to build up a json structure or dump it
     * elsewhere. We could have made a "to_json" method, but that would
     * add a dependency to cJSON from our platform lib (this method
     * is intended to be called from memcached core on the connections
     * read and write buffer for connection stats).
     */
    void stats(std::function<void(const char* /* key */,
                                  const char* /* value */)> stats);

    /**
     * The underlying buffer is being allocated by a number of chunks of the
     * given size. The input size of the buffer is rounded to sizes of this
     * size, and the chunk size for the pipe is set to that.
     */
    static const size_t defaultAllocationChunkSize;

protected:
    /**
     * Get information of the _unused_ space in the write end of
     * the pipe. This is a contiguous space the caller may use, and
     * call produced() later on to mark the space as used and that it
     * should be available for the consumer in the read end of the pipe.
     */
    cb::byte_buffer getAvailableWriteSpace() const {
        return {const_cast<uint8_t*>(buffer.data()) + write_head,
                buffer.size() - write_head};
    }

    /**
     * A number of bytes was made available for the consumer
     */
    void produced(size_t nbytes);

    /**
     * Get information of the available data the consumer may read.
     * This is a contiguous space the caller may
     * use, and call consumed() later on to mark the space as free and
     * available for the producer.
     */
    cb::const_byte_buffer getAvailableReadSpace() const {
        return {buffer.data() + read_head, write_head - read_head};
    }

    /**
     * The number of bytes just removed from the consumer end of the buffer.
     *
     * If the consumer catch up with the producer all pointers returned
     * could be invalidated (most likely reset to the beginning of the
     * buffer)
     */
    void consumed(size_t nbytes);

    /*
     * Some helper functions while running under Valgrind to help us
     * verify that people aren't messing around with the buffers when
     * they shouldn't.
     *
     * The documentation in the manual is a bit vague on how the underlying
     * stuff works, so I decided to take the simple approach. In the "normal"
     * situation we keep the section of the data containing data we may read
     * open for access. The rest of the buffer is locked. When the client
     * calls the "produce" methods, we lock the read buffer and open up
     * the write buffer for access.
     */
#ifdef CB_PIPE_VALGRIND_INTEGRATION
    void valgrind_unlock_entire_buffer();
    void valgrind_lock_entire_buffer();

    void valgrind_unlock_write_buffer();
    void valgrind_lock_write_buffer();

    void valgrind_unlock_read_buffer();
    void valgrind_lock_read_buffer();
#else
    void valgrind_unlock_entire_buffer() {}
    void valgrind_lock_entire_buffer() {}

    void valgrind_unlock_write_buffer() {}
    void valgrind_lock_write_buffer() {}

    void valgrind_unlock_read_buffer() {}
    void valgrind_lock_read_buffer() {}
#endif

    // The information about the underlying buffer
    cb::byte_buffer buffer;

    std::unique_ptr<uint8_t[]> memory;

    // The offset in the buffer where we may start write
    size_t write_head = 0;

    // The offset in the buffer where we may start deading
    size_t read_head = 0;

    // Is the pipe locked or not (for testing)
    bool locked = false;

    // the allocation chunk size.
    const size_t allocation_chunk_size;
};

} // namespace cb
