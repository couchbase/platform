/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/cb_malloc.h>

#include <new>
#include <stdexcept>

namespace cb::compression {

/**
 * The memory allocator allows you to choose if you want the memory to
 * be allocated with cb_malloc/cb_free or new/delete. It should probably be
 * reimplemented with the fancier Allocator stuff in C++, but from a quick
 * glance that looked a bit more complex than what's needed for now..
 */
struct Allocator {
    enum class Mode {
        /**
         * Use the new array allocator to allocate backing space.
         * The buffer must be released with delete[] if the memory
         * is released from the buffer
         */
        New,
        /**
         * Use cb_malloc to allocate backing space. The memory must
         * be freed with cb_free if the memory is released from the buffer
         */
        Malloc
    };

    explicit Allocator(Mode mode_ = Mode::New) : mode(mode_) {
    }

    char* allocate(size_t nbytes) {
        char* ret;

        switch (mode) {
        case Mode::New:
            return new char[nbytes];
        case Mode::Malloc:
            ret = static_cast<char*>(cb_malloc(nbytes));
            if (ret == nullptr) {
                throw std::bad_alloc();
            }
            return ret;
        }
        throw std::runtime_error("Allocator::allocate: Unknown mode");
    }

    void deallocate(char* ptr) {
        switch (mode) {
        case Mode::New:
            delete[] ptr;
            return;
        case Mode::Malloc:
            cb_free(static_cast<void*>(ptr));
            return;
        }
        throw std::runtime_error("Allocator::deallocate: Unknown mode");
    }

    const Mode mode;
};
} // namespace cb::compression
