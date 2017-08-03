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
#include <platform/make_unique.h>
#include <platform/pipe.h>

#include <algorithm>
#include <cinttypes>

namespace cb {

static size_t calculateAllocationChunkSize(size_t nbytes) {
    auto chunks = nbytes / Pipe::defaultAllocationChunkSize;
    if (nbytes % Pipe::defaultAllocationChunkSize != 0) {
        chunks++;
    }

    return chunks * Pipe::defaultAllocationChunkSize;
}

const size_t Pipe::defaultAllocationChunkSize = 512;

Pipe::Pipe(size_t size)
    : allocation_chunk_size(std::max(calculateAllocationChunkSize(size),
                                     size_t(defaultAllocationChunkSize))) {
    memory.reset(new uint8_t[size]);
    buffer = {memory.get(), size};
}

size_t Pipe::ensureCapacity(size_t nbytes) {
    if (locked) {
        throw std::logic_error("Pipe::ensureCapacity(): Buffer locked");
    }

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
                    "Pipe::ensureCapacity: expecting pack to free up enough "
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
    std::copy(memory.get() + read_head, memory.get() + write_head, nmem.get());
    memory.swap(nmem);
    buffer = {memory.get(), nsize};
    write_head -= read_head;
    read_head = 0;

    return wsize();
}

bool Pipe::pack() {
    if (locked) {
        throw std::logic_error("Pipe::pack(): Buffer locked");
    }

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

ssize_t Pipe::consume(std::function<ssize_t(const void* /* ptr */,
                                            size_t /* size */)> consumer) {
    if (locked) {
        throw std::logic_error("Pipe::consume(): Buffer locked");
    }
    auto avail = getAvailableReadSpace();
    const ssize_t ret =
            consumer(static_cast<const void*>(avail.data()), avail.size());
    if (ret > 0) {
        consumed(ret);
    }
    return ret;
}

ssize_t Pipe::consume(std::function<ssize_t(cb::const_byte_buffer)> consumer) {
    if (locked) {
        throw std::logic_error("Pipe::consume(): Buffer locked");
    }
    auto avail = getAvailableReadSpace();
    const ssize_t ret = consumer({avail.data(), avail.size()});
    if (ret > 0) {
        consumed(ret);
    }
    return ret;
}

ssize_t Pipe::produce(
        std::function<ssize_t(void* /* ptr */, size_t /* size */)> producer) {
    if (locked) {
        throw std::logic_error("Pipe::produce(): Buffer locked");
    }
    auto avail = getAvailableWriteSpace();

    const ssize_t ret =
            producer(static_cast<void*>(avail.data()), avail.size());

    if (ret > 0) {
        produced(ret);
    }

    return ret;
}

ssize_t Pipe::produce(std::function<ssize_t(cb::byte_buffer)> producer) {
    if (locked) {
        throw std::logic_error("Pipe::produce(): Buffer locked");
    }
    auto avail = getAvailableWriteSpace();

    const ssize_t ret = producer({avail.data(), avail.size()});

    if (ret > 0) {
        produced(ret);
    }

    return ret;
}

void Pipe::stats(std::function<void(const char* /* key */,
                                    const char* /* value */)> stats) {
    char value[64];
    snprintf(value, sizeof(value), "0x%" PRIxPTR, uintptr_t(buffer.data()));
    stats("buffer", value);
    stats("size", std::to_string(buffer.size()).c_str());
    stats("read_head", std::to_string(read_head).c_str());
    stats("write_head", std::to_string(write_head).c_str());
    stats("empty", empty() ? "true" : "false");
    stats("locked", locked ? "true" : "false");
}

} // namespace cb
