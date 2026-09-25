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
#include <cstddef>
#include <string>
#include <string_view>

namespace cb::compression {

/**
 * All data inside kv-engine (and on the wire) use Snappy compression.
 * This is a wrapper method used to save some typing ;)
 */
[[nodiscard]] bool inflateSnappy(std::string_view input,
                                 Buffer& output,
                                 size_t max_inflated_size);

/**
 * Inflate Snappy-compressed data and return it as a std::string
 *
 * @param input The data to inflate
 * @param max_inflated_size The maximum size for the inflated object (the
 *                          library needs to allocate buffers this big, which
 *                          could affect other components in the system. If
 *                          the resulting object becomes bigger than this
 *                          limit we'll abort and return false)
 * @return The inflated data
 * @throws std::bad_alloc if allocation fails for the destination buffer
 * @throws std::range_error if the inflated data would exceed max_inflated_size
 * @throws std::runtime_error if there is an error related to inflating data
 */
[[nodiscard]] std::string inflateSnappy(std::string_view input,
                                        size_t max_inflated_size);

/**
 * All data inside kv-engine (and on the wire) use Snappy compression.
 * This is a wrapper method used to save some typing ;)
 */
[[nodiscard]] bool deflateSnappy(std::string_view input, Buffer& output);

/**
 * Deflate the data with Snappy and return it as a std::string
 *
 * @param input The data to deflate
 * @return The deflated data
 * @throws std::bad_alloc if allocation fails for the destination buffer
 */
[[nodiscard]] std::string deflateSnappy(std::string_view input);

/**
 * Get the uncompressed length from the given Snappy compressed input buffer
 *
 * @param input buffer pointing to the input buffer
 * @return the uncompressed length if success, false otherwise
 */
[[nodiscard]] size_t getUncompressedLengthSnappy(std::string_view input);

} // namespace cb::compression
