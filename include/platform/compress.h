/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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

#include <memory>
#include <platform/compress-visibility.h>
#include <platform/sized_buffer.h>

namespace cb {
namespace compression {
enum class Algorithm { Snappy };

struct CBCOMPRESS_PUBLIC_API Buffer {
    Buffer() : len(0) {
    }
    std::unique_ptr<char[]> data;
    size_t len;

    operator const_byte_buffer() const {
        return const_byte_buffer(reinterpret_cast<const uint8_t*>(data.get()),
                                 len);
    }
};

/**
 * Inflate the data in the buffer into the output buffer
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input data
 * @param output Where to store the result
 * @return true if success, false otherwise
 * @throws std::bad_alloc if we fail to allocate memory for the
 *                        destination buffer
 */
CBCOMPRESS_PUBLIC_API
bool inflate(const Algorithm algorithm,
             cb::const_char_buffer input_buffer,
             Buffer& output);

/**
 * Deflate the data in the buffer into the output buffer
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input data
 * @param output Where to store the result
 * @return true if success, false otherwise
 * @throws std::bad_alloc if we fail to allocate memory for the
 *                        destination buffer
 */
CBCOMPRESS_PUBLIC_API
bool deflate(const Algorithm algorithm,
             cb::const_char_buffer input_buffer,
             Buffer& output);

/**
 * Validate whether the data is compressed correctly by the given
 * algorithm
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input data
 * @return true if success, false otherwise
 * @throws std::invalid_argument if the algorithm provided is an
 *                               an unknown algorithm
 */
CBCOMPRESS_PUBLIC_API
bool validate(const Algorithm algorithm,
              cb::const_char_buffer input_buffer);
}
}
