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
#include <platform/make_unique.h>
#include <platform/platform.h>
#include <platform/sized_buffer.h>

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>

namespace cb {

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
class Pipe {
public:
    /**
     * Initialize a pipe with the given buffer size (default empty).
     *
     * The write end of the pipe may be increased by calling ensureCapacity.
     *
     * @param size The initial size of the buffer in the pipe (default 0)
     */
    explicit Pipe(size_t size = 0)
        : allocation_chunk_size(
                  std::max(calculateAllocationChunkSize(size), size_t(512))) {
        memory.reset(new uint8_t[size]);
        buffer = {memory.get(), size};
    }

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
    size_t ensureCapacity(size_t nbytes) {
        const size_t tail_space = buffer.size() - write_head;
        if (tail_space >= nbytes) { // do we have enough space at the tail?
            return wsize();
        }

        if (nbytes <= (tail_space + read_head)) {
            // we've got space if we pack the buffer
            const auto head_space = read_head;
            pack();
            auto ret = wsize();
            if (ret < nbytes) {
                throw std::logic_error(
                        "Pipe::ensureCapacity: expecting pack to free up "
                        "enough "
                        "bytes: " +
                        std::to_string(ret) + " < " + std::to_string(nbytes) +
                        ". hs: " + std::to_string(head_space) +
                        " ts: " + std::to_string(tail_space));
            }
            return ret;
        }

        // No. We need to allocate a bigger memory area to fit the requested
        size_t count = 1;
        while (((count * allocation_chunk_size) + tail_space + read_head) <
               nbytes) {
            ++count;
        }

        const auto nsize = buffer.size() + (count * allocation_chunk_size);
        std::unique_ptr<uint8_t[]> nmem(new uint8_t[nsize]);

        // Copy the existing data over
        std::copy(memory.get() + read_head,
                  memory.get() + write_head,
                  nmem.get());
        memory.swap(nmem);
        buffer = {memory.get(), nsize};
        write_head -= read_head;
        read_head = 0;

        return wsize();
    }

    /**
     * Get the current allocation size of the buffer
     */
    size_t capacity() const {
        return buffer.size();
    }

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
     * Get the available write buffer
     */
    cb::byte_buffer wdata() const {
        return getAvailableWriteSpace();
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
                            producer) {
        auto avail = getAvailableWriteSpace();

        const ssize_t ret =
                producer(static_cast<void*>(avail.data()), avail.size());

        if (ret > 0) {
            produced(ret);
        }

        return ret;
    }

    /**
     * Try to produce a number of bytes by providing a callback function
     * which will receive the buffer where the data may be inserted
     *
     * @param producer a callback function to produce data into the
     *                 provided buffer.
     * @return the number of bytes produced
     */
    ssize_t produce(std::function<ssize_t(cb::byte_buffer)> producer) {
        auto avail = getAvailableWriteSpace();

        const ssize_t ret = producer({avail.data(), avail.size()});

        if (ret > 0) {
            produced(ret);
        }

        return ret;
    }

    /**
     * A number of bytes was made available for the consumer
     */
    void produced(size_t nbytes) {
        if (write_head + nbytes > buffer.size()) {
            throw std::logic_error(
                    "Pipe::produced(): Produced bytes exceeds "
                    "the number of available bytes");
        }
        write_head += nbytes;
    }

    /**
     * Try to consume data from the buffer by providing a callback function
     *
     * @param producer a callback function to consume data from the provided
     *                 continuous memory area from ptr and size bytes long.
     *                 The number of bytes consumed should be returned.
     * @return the number of bytes consumed
     */
    ssize_t consume(std::function<ssize_t(const void* /* ptr */,
                                          size_t /* size */)> consumer) {
        auto avail = getAvailableReadSpace();
        const ssize_t ret =
                consumer(static_cast<const void*>(avail.data()), avail.size());
        if (ret > 0) {
            consumed(ret);
        }
        return ret;
    }

    /**
     * Try to consume data from the buffer by providing a callback function
     *
     * @param producer a callback function to consume data from the provided
     *                 memory area. The number of bytes consumed should be
     *                 returned.
     * @return the number of bytes consumed
     */
    ssize_t consume(std::function<ssize_t(cb::const_byte_buffer)> consumer) {
        auto avail = getAvailableReadSpace();
        const ssize_t ret = consumer({avail.data(), avail.size()});
        if (ret > 0) {
            consumed(ret);
        }
        return ret;
    }

    /**
     * The number of bytes just removed from the consumer end of the buffer.
     *
     * If the consumer catch up with the producer all pointers returned
     * could be invalidated (most likely reset to the beginning of the
     * buffer)
     */
    void consumed(size_t nbytes) {
        if (read_head + nbytes > write_head) {
            throw std::logic_error(
                    "Pipe::consumed(): Consumed bytes exceeds "
                    "the number of available bytes");
        }

        read_head += nbytes;
        if (empty()) {
            read_head = write_head = 0;
        }
    }

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
    bool pack() {
        if (read_head == write_head) {
            read_head = write_head = 0;
        } else {
            ::memmove(buffer.data(),
                      buffer.data() + read_head,
                      write_head - read_head);
            write_head -= read_head;
            read_head = 0;
        }

        return empty();
    }

    /**
     * Is this buffer empty (the consumer end completely caught up with
     * the producer)
     */
    bool empty() const {
        return read_head == write_head;
    }

    /**
     * Is this buffer full or not
     */
    bool full() const {
        return write_head == buffer.size();
    }

    /**
     * Clear all of the content in the buffer
     */
    void clear() {
        write_head = read_head = 0;
    }

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
                                  const char* /* value */)> stats) {
        char value[64];
        snprintf(value, sizeof(value), "0x%" PRIxPTR, uintptr_t(buffer.data()));
        stats("buffer", value);
        stats("size", std::to_string(buffer.size()).c_str());
        stats("read_head", std::to_string(read_head).c_str());
        stats("write_head", std::to_string(write_head).c_str());
        stats("empty", empty() ? "true" : "false");
    }

protected:
    size_t calculateAllocationChunkSize(size_t nbytes) {
        auto chunks = nbytes / 512;
        if (nbytes % 512 != 0) {
            chunks++;
        }

        return chunks * 512;
    }

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
     * Get information of the available data the consumer may read.
     * This is a contiguous space the caller may
     * use, and call consumed() later on to mark the space as free and
     * available for the producer.
     */
    cb::const_byte_buffer getAvailableReadSpace() const {
        return {buffer.data() + read_head, write_head - read_head};
    }

    // The information about the underlying buffer
    cb::byte_buffer buffer;

    std::unique_ptr<uint8_t[]> memory;

    // The offset in the buffer where we may start write
    size_t write_head = 0;

    // The offset in the buffer where we may start deading
    size_t read_head = 0;

    // the allocation chunk size.
    const size_t allocation_chunk_size;
};

} // namespace cb
