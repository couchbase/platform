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

#include <platform/compress-visibility.h>
#include <platform/compression/allocator.h>
#include <platform/sized_buffer.h>
#include <memory>

namespace cb {
namespace compression {

/**
 * The compression API needs to allocate the output buffer during
 * compression / decmpression, and keep the size around.
 *
 * We've got code in our system which use new/delete for allocating
 * buffers, and some old code which still use cb_malloc/cb_free. We
 * can't unfortunately mix and mach new/cb_free etc, and we don't
 * want to reallocate the memory after using this method. That's
 * why the Buffer class handle both the new and malloc internally.
 *
 */
class CBCOMPRESS_PUBLIC_API Buffer {
public:
    /**
     * Initialize a new buffer which use the requested memory allocator
     * for memory allocation.
     */
    explicit Buffer(Allocator alloc = Allocator{})
        : allocator(alloc), memory(nullptr, FreeDeleter(allocator)) {
    }

    /**
     * Resize the underlying buffer. If the buffer grows to a bigger size
     * the content of the allocated memory is undefined.
     *
     * @param sz The new size for the buffer
     * @throws std::bad_alloc if we failed to allocate memory
     */
    void resize(size_t sz) {
        if (sz > capacity_) {
            memory.reset(allocator.allocate(sz));
            capacity_ = sz;
        }
        size_ = sz;
    }

    /**
     * Get a pointer to the backing storage for the buffer. The data area
     * is a continuous memory space size() bytes big.
     */
    char* data() {
        return memory.get();
    }

    /**
     * Get a pointer to the backing storage for the buffer. The data area
     * is a continuous memory space size() bytes big.
     */
    const char* data() const {
        return memory.get();
    }

    /**
     * Release / detach / take ownership of the underlying buffer
     *
     * The caller is now responsible for freeing the memory by using
     * the allocator's deallocate method
     */
    char* release() {
        auto* ret = memory.release();
        reset();
        return ret;
    }

    /**
     * Get the current size of the buffer
     */
    size_t size() const {
        return size_;
    }

    /// Is this buffer empty or not
    bool empty() const {
        return size_ == 0;
    }

    /**
     * Get the capacity of the buffer
     */
    size_t capacity() const {
        return capacity_;
    }

    /**
     * Reset the buffer (and free up all resources)
     */
    void reset() {
        memory.reset();
        capacity_ = size_ = 0;
    }

    /**
     * Convenience operator to get a sized buffer pointing into the
     * buffer owned by this buffer
     */
    operator cb::char_buffer() {
        return cb::char_buffer{data(), size()};
    }

    /**
     * Convenience operator to get a sized buffer pointing into the
     * buffer owned by this buffer
     */
    operator cb::const_char_buffer() const {
        return cb::const_char_buffer{data(), size()};
    }

    operator cb::const_byte_buffer() const {
        return cb::const_byte_buffer{reinterpret_cast<const uint8_t*>(data()),
                                     size()};
    }

    Allocator allocator;

private:
    size_t capacity_ = 0;
    size_t size_ = 0;

    struct FreeDeleter {
        FreeDeleter() = delete;
        explicit FreeDeleter(Allocator& alloc) : allocator(alloc) {
        }
        void operator()(char* ptr) {
            allocator.deallocate(ptr);
        }

        Allocator& allocator;
    };
    std::unique_ptr<char, FreeDeleter> memory;
};

} // namespace compression
} // namespace cb
