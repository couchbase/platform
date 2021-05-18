/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

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
bool deflate(Algorithm algorithm,
             std::string_view input_buffer,
             Buffer& output);

/**
 * Get the algorithm as specified by the textual string
 */
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
size_t get_uncompressed_length(Algorithm algorithm,
                               std::string_view input_buffer);
} // namespace cb::compression

std::string to_string(cb::compression::Algorithm algorithm);
