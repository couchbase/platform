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

#include <valgrind/memcheck.h>

#include <cinttypes>

namespace cb {

Pipe::Pipe(size_t size) {
    memory.reset(new uint8_t[size]);
    buffer = {memory.get(), size};
    // Make the entire buffer inaccessible!
    valgrind_lock_entire_buffer();
    // but the read space should be open..
    valgrind_unlock_read_buffer();
}

size_t Pipe::ensureCapacity(size_t nbytes) {
    if (locked) {
        throw std::logic_error("Pipe::ensureCapacity(): Buffer locked");
    }

    // Move data to the beginning of the segment
    pack();

    const size_t avail = buffer.size() - write_head;
    if (avail < nbytes) {
        // Allocate a (potentially) slightly bigger area (ignore the current
        // available space in the existing buffer). That'll (hopefully) be
        // used in a later allocation.
        const auto nsize = buffer.size() + nbytes;
        std::unique_ptr<uint8_t[]> nmem(new uint8_t[nsize]);

        // Make the entire buffer unlocked..
        valgrind_lock_read_buffer();
        valgrind_unlock_entire_buffer();

        // Copy the existing data over
        std::copy(memory.get(), memory.get() + write_head, nmem.get());
        memory.swap(nmem);
        buffer = {memory.get(), nsize};

        // Make the entire buffer inaccessible!
        valgrind_lock_entire_buffer();
        // but the read space should be open..
        valgrind_unlock_read_buffer();
    }

    return getAvailableWriteSpace().size();
}

cb::byte_buffer Pipe::getAvailableWriteSpace() const {
    return {const_cast<uint8_t*>(buffer.data()) + write_head,
            buffer.size() - write_head};
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

    // We've added more data into the read end..
    valgrind_lock_read_buffer();

    write_head += nbytes;

    // Open up the read buffer with the extended segment
    valgrind_unlock_read_buffer();
}

cb::const_byte_buffer Pipe::getAvailableReadSpace() const {
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

    // We've removed data from the read end so we need to move our
    // read window.

    valgrind_lock_read_buffer();

    read_head += nbytes;
    if (read_head == write_head) {
        read_head = write_head = 0;
    }

    valgrind_unlock_read_buffer();
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

    valgrind_lock_read_buffer();
    valgrind_unlock_entire_buffer();

    if (read_head == write_head) {
        read_head = write_head = 0;
    } else {
        ::memmove(buffer.data(),
                  buffer.data() + read_head,
                  write_head - read_head);
        write_head -= read_head;
        read_head = 0;
    }

    valgrind_lock_entire_buffer();
    valgrind_unlock_read_buffer();

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

    valgrind_lock_read_buffer();
    valgrind_unlock_write_buffer();

    const ssize_t ret =
            producer(static_cast<void*>(avail.data()), avail.size());

    valgrind_lock_write_buffer();
    valgrind_unlock_read_buffer();

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

    valgrind_lock_read_buffer();
    valgrind_unlock_write_buffer();

    const ssize_t ret = producer({avail.data(), avail.size()});

    valgrind_lock_write_buffer();
    valgrind_unlock_read_buffer();

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

void Pipe::valgrind_unlock_entire_buffer() {
    if (!buffer.empty()) {
        VALGRIND_MAKE_MEM_DEFINED(buffer.data(), buffer.size());
    }
}

void Pipe::valgrind_lock_entire_buffer() {
    if (!buffer.empty()) {
        VALGRIND_MAKE_MEM_NOACCESS(buffer.data(), buffer.size());
    }
}

void Pipe::valgrind_unlock_write_buffer() {
    auto avail = getAvailableWriteSpace();
    if (!avail.empty()) {
        VALGRIND_MAKE_MEM_DEFINED(avail.data(), avail.size());
    }
}

void Pipe::valgrind_lock_write_buffer() {
    auto avail = getAvailableWriteSpace();
    if (!avail.empty()) {
        VALGRIND_MAKE_MEM_NOACCESS(avail.data(), avail.size());
    }
}

void Pipe::valgrind_unlock_read_buffer() {
    auto avail = getAvailableReadSpace();
    if (!avail.empty()) {
        VALGRIND_MAKE_MEM_DEFINED(avail.data(), avail.size());
    }
}

void Pipe::valgrind_lock_read_buffer() {
    auto avail = getAvailableReadSpace();
    if (!avail.empty()) {
        VALGRIND_MAKE_MEM_NOACCESS(avail.data(), avail.size());
    }
}

size_t Pipe::rsize() const {
    return getAvailableReadSpace().size();
}

size_t Pipe::wsize() const {
    return getAvailableWriteSpace().size();
}

} // namespace cb
