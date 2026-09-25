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
#include <memory>
#include <string_view>

namespace folly {
class IOBuf;
}

namespace cb::compression {

/**
 * All data inside kv-engine (and on the wire) use Snappy compression.
 * This is a wrapper method used to save some typing ;)
 */
[[nodiscard]] bool inflateSnappy(std::string_view input,
                                 Buffer& output,
                                 size_t max_inflated_size);

[[nodiscard]] std::unique_ptr<folly::IOBuf> inflateSnappy(
        std::string_view input, size_t max_inflated_size);

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
