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

#include <platform/compress-visibility.h>
#include <platform/compression/buffer.h>

namespace cb::compression {
enum class Algorithm { Snappy, LZ4 };
/**
 * The default maximum size used during inflating of buffers to avoid having
 * the library go ahead and allocate crazy big sizes if the input is
 * garbled which could impact the rest of the system.
 */
static const size_t DEFAULT_MAX_INFLATED_SIZE = 30 * 1024 * 1024;

/**
 * Inflate the data in the buffer into the output buffer
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input data
 * @param output Where to store the result
 * @param max_inflated_size The maximum size for the inflated object (the
 *                          library needs to allocate buffers this big, which
 *                          could affect other components in the system. If
 *                          the resulting object becomes bigger than this
 *                          limit we'll abort and return false)
 * @return true if success, false otherwise
 * @throws std::bad_alloc if we fail to allocate memory for the
 *                        destination buffer
 */
CBCOMPRESS_PUBLIC_API
bool inflate(Algorithm algorithm,
             std::string_view input_buffer,
             Buffer& output,
             size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

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
bool deflate(Algorithm algorithm,
             std::string_view input_buffer,
             Buffer& output);

/**
 * Get the algorithm as specified by the textual string
 */
CBCOMPRESS_PUBLIC_API
Algorithm to_algorithm(const std::string& string);

/**
 * Validate whether the data is compressed correctly by the given
 * algorithm
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input data
 * @param max_inflated_size The maximum size for the inflated object (if the
 *                          library needs to allocate buffers exceeding this
 *                          size in order to validate the input, we'll abort
 *                          and return false)
 * @return true if success, false otherwise
 * @throws std::invalid_argument if the algorithm provided is an
 *                               an unknown algorithm
 */
CBCOMPRESS_PUBLIC_API
bool validate(Algorithm algorithm,
              std::string_view input_buffer,
              size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

/**
 * Get the uncompressed length from the given compressed input buffer
 *
 * @param algorithm the algorithm to use
 * @param input_buffer buffer pointing to the input buffer
 * @return the uncompressed length if success, false otherwise
 * @throws std::invalid_argument if the algorithm provided is an
 *                               unknown algorithm
 */
CBCOMPRESS_PUBLIC_API
size_t get_uncompressed_length(Algorithm algorithm,
                               std::string_view input_buffer);
} // namespace cb::compression

CBCOMPRESS_PUBLIC_API
std::string to_string(cb::compression::Algorithm algorithm);
