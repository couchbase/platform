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

#include <cinttypes>

namespace cb {

Pipe::Pipe(size_t size) {
    memory.reset(cb_malloc(size));
    buffer = {static_cast<uint8_t*>(memory.get()), size};
}

size_t Pipe::ensureCapacity(size_t nbytes) {
    if (locked) {
        throw std::logic_error("Pipe::ensureCapacity(): Buffer locked");
    }

    const auto tail_size = buffer.size() - write_head;

    if (tail_size < nbytes) {
        // We might need to resize the buffer if we can't move stuff around...
        const size_t avail = read_head + buffer.size() - write_head;
        if (nbytes > avail) {
            const auto nsize = buffer.size() + nbytes;
            memory.reset(cb_realloc(memory.get(), nsize));
            buffer = {static_cast<uint8_t*>(memory.get()), nsize};
        } else {
            // we've got space inside the buffer if we just pack the bytes
            pack();
        }
    }

    return getAvailableWriteSpace().size();
}

cb::byte_buffer Pipe::getAvailableWriteSpace() {
    return {buffer.data() + write_head, buffer.size() - write_head};
}

void Pipe::produced(size_t nbytes) {
    if (locked) {
        throw std::logic_error("Pipe::produced(): Buffer locked");
    }

    if (write_head + nbytes > buffer.size()) {
        throw std::logic_error(
                "Pipe::produced(): Produced bytes exceeds "
                "the number of available bytes");
    }

    write_head += nbytes;
}

cb::const_byte_buffer Pipe::getAvailableReadSpace() {
    return {buffer.data() + read_head, write_head - read_head};
}

void Pipe::consumed(size_t nbytes) {
    if (locked) {
        throw std::logic_error("Pipe::consumed(): Buffer locked");
    }

    if (read_head + nbytes > write_head) {
        throw std::logic_error(
                "Pipe::consumed(): Consumed bytes exceeds "
                "the number of available bytes");
    }

    read_head += nbytes;
    if (read_head == write_head) {
        read_head = write_head = 0;
    }
}

bool Pipe::empty() const {
    return read_head == write_head;
}

bool Pipe::full() const {
    return write_head == buffer.size();
}

void Pipe::lock() {
    if (locked) {
        throw std::logic_error("Pipe::lock(): Buffer already locked");
    }
    locked = true;
}

void Pipe::unlock() {
    if (!locked) {
        throw std::logic_error("Pipe::unlock(): Buffer not locked");
    }
    locked = false;
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

void Pipe::clear() {
    if (locked) {
        throw std::logic_error("Pipe::clear(): Buffer locked");
    }
    write_head = read_head = 0;
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
