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

#include <folly/compression/Compression.h>
#include <platform/compression/buffer.h>
#include <memory>

namespace folly {
class IOBuf;
}

namespace cb::compression {

/**
 * The default maximum size used during inflating of buffers to avoid having
 * the library go ahead and allocate crazy big sizes if the input is
 * garbled which could impact the rest of the system.
 */
static const size_t DEFAULT_MAX_INFLATED_SIZE = 30 * 1024 * 1024;

/**
 * Inflate the data in the buffer into the output buffer
 *
 * @param type The codec to use (currently ignored)
 * @param input buffer pointing to the input data
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
[[nodiscard]] bool inflate(
        folly::io::CodecType type,
        std::string_view input,
        Buffer& output,
        size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

/**
 * Inflate the data and return a std::unique_ptr to a folly IOBuf
 * containing the inflated data
 *
 * @param type The codec to use (currently ignored)
 * @param input The data to inflate
 * @param max_inflated_size The maximum size for the inflated object (the
 *                          library needs to allocate buffers this big, which
 *                          could affect other components in the system. If
 *                          the resulting object becomes bigger than this
 *                          limit we'll abort and return false)
 * @return The inflated data
 * @throws std::invalid_argument for unsupported CodecTypes
 * @throws std::bad_alloc if allocation fails for the destination buffer
 * @throws std::range_error if the inflated data would exceed max_inflated_size
 * @throws std::runtime_error if there is an error related to inflating data
 */
[[nodiscard]] std::unique_ptr<folly::IOBuf> inflate(
        folly::io::CodecType type,
        std::string_view input,
        size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

/**
 * Deflate the data in the buffer into the output buffer
 *
 * @param type The codec type to use
 * @param input_buffer buffer pointing to the input data
 * @param output Where to store the result
 * @return true if success, false otherwise
 * @throws std::bad_alloc if we fail to allocate memory for the
 *                        destination buffer
 */
[[nodiscard]] bool deflate(folly::io::CodecType type,
                           std::string_view input_buffer,
                           Buffer& output);

/**
 * Deflate the data and return a std::unique_ptr to a folly IOBuf
 * containing the deflated data
 *
 * @param type The codec to use (currently ignored)
 * @param input The data to deflate
 * @return The deflated data
 * @throws std::invalid_argument for unsupported CodecTypes
 * @throws std::bad_alloc if allocation fails for the destination buffer
 * @throws std::runtime_error if there is an error related to deflating data
 */
[[nodiscard]] std::unique_ptr<folly::IOBuf> deflate(folly::io::CodecType type,
                                                    std::string_view input);

/**
 * Get the uncompressed length from the given compressed input buffer
 *
 * @param type The codec type to use
 * @param input buffer pointing to the input buffer
 * @return the uncompressed length if success, false otherwise
 * @throws std::invalid_argument if the algorithm provided is an
 *                               unknown algorithm
 */
[[nodiscard]] size_t get_uncompressed_length(folly::io::CodecType type,
                                             std::string_view input);

/**
 * All data inside kv-engine (and on the wire) use Snappy compression.
 * This is a wrapper method used to save some typing ;)
 */
[[nodiscard]] bool inflateSnappy(
        std::string_view input,
        Buffer& output,
        size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

[[nodiscard]] std::unique_ptr<folly::IOBuf> inflateSnappy(
        std::string_view input,
        size_t max_inflated_size = DEFAULT_MAX_INFLATED_SIZE);

/**
 * All data inside kv-engine (and on the wire) use Snappy compression.
 * This is a wrapper method used to save some typing ;)
 */
[[nodiscard]] bool deflateSnappy(std::string_view input, Buffer& output);

[[nodiscard]] std::unique_ptr<folly::IOBuf> deflateSnappy(
        std::string_view input);

/**
 * Get the uncompressed length from the given Snappy compressed input buffer
 *
 * @param input buffer pointing to the input buffer
 * @return the uncompressed length if success, false otherwise
 */
[[nodiscard]] size_t getUncompressedLengthSnappy(std::string_view input);

} // namespace cb::compression
