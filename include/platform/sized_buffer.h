/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

#include <cstddef>
#include <cstdint>
#include <string>

namespace cb {

/**
 * Struct representing a buffer of some known size. This is used to
 * refer to some existing region of memory which is owned elsewhere - i.e.
 * this object does not have ownership semantics.
 * A user should not free() the buf member themselves!
 */
template <typename T>
struct sized_buffer {
    sized_buffer() : sized_buffer(nullptr, 0) {
    }

    sized_buffer(T* buf_, size_t len_) : buf(buf_), len(len_) {
    }

    T* data() {
        return buf;
    }

    const T* data() const {
        return buf;
    }

    size_t size() const {
        return len;
    }

    T* buf;
    size_t len;
};

/**
 * The char_buffer is intended for strings you might want to modify
 */
using char_buffer = sized_buffer<char>;

/**
 * The const_char_buffer is intended for strings you're not going
 * to modify. I need to use inheritance in order to add the constructor
 * to initialize from a std::string (as std::enable_if doesn't seem to
 * play very well on constructors which don't have a return value...
 * If you know how to make it work feel free to fix it)
 */
struct const_char_buffer : public sized_buffer<const char> {
    const_char_buffer()
        : sized_buffer() {
    }

    const_char_buffer(const char* buf_, size_t len_)
        : sized_buffer(buf_, len_) {
    }

    const_char_buffer(const std::string& str)
        : sized_buffer(str.data(), str.size()) {}
};

/**
 * The byte_buffer is intended to use for a blob of bytes you might want
 * to modify
 */
using byte_buffer = sized_buffer<uint8_t>;

/**
 * The const_byte_buffer is intended for a blob of bytes you _don't_
 * want to modify.
 */
using const_byte_buffer = sized_buffer<const uint8_t>;
}
