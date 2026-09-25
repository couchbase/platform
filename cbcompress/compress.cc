/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>
#include <platform/compress.h>
#include <snappy.h>

#include <stdexcept>

namespace cb::compression {
bool inflateSnappy(std::string_view input,
                   Buffer& output,
                   size_t max_inflated_size) {
    size_t inflated_length;

    if (!snappy::GetUncompressedLength(
                input.data(), input.size(), &inflated_length)) {
        return false;
    }

    if (inflated_length > max_inflated_size) {
        return false;
    }

    output.resize(inflated_length);
    if (!snappy::RawUncompress(input.data(), input.size(), output.data())) {
        return false;
    }
    return true;
}

std::string inflateSnappy(std::string_view input, size_t max_inflated_size) {
    size_t inflated_length;
    if (!snappy::GetUncompressedLength(
                input.data(), input.size(), &inflated_length)) {
        throw std::runtime_error(
                "cb::compression::inflateSnappy(): Failed to get uncompressed "
                "length");
    }

    if (inflated_length > max_inflated_size) {
        throw std::range_error(
                fmt::format("cb::compression::inflate(): Inflated length {} "
                            "would exceed max: {}",
                            inflated_length,
                            max_inflated_size));
    }

    bool success = false;
    std::string ret;
    ret.resize_and_overwrite(
            inflated_length, [&input, &success](char* data, size_t size) {
                success =
                        snappy::RawUncompress(input.data(), input.size(), data);
                return size;
            });
    if (!success) {
        throw std::runtime_error(
                "cb::compression::inflateSnappy: Failed to inflate data");
    }
    return ret;
}

bool deflateSnappy(std::string_view input, Buffer& output) {
    size_t compressed_length = snappy::MaxCompressedLength(input.size());
    output.resize(compressed_length);
    snappy::RawCompress(
            input.data(), input.size(), output.data(), &compressed_length);
    output.resize(compressed_length);
    return true;
}

std::string deflateSnappy(std::string_view input) {
    const size_t max_compressed_length =
            snappy::MaxCompressedLength(input.size());
    size_t compressed_length = max_compressed_length;
    std::string ret;
    ret.resize_and_overwrite(
            max_compressed_length,
            [&input, &compressed_length](char* data, size_t size) {
                snappy::RawCompress(
                        input.data(), input.size(), data, &compressed_length);
                return compressed_length;
            });
    return ret;
}

size_t getUncompressedLengthSnappy(std::string_view input) {
    size_t uncompressed_length = 0;
    snappy::GetUncompressedLength(
            input.data(), input.size(), &uncompressed_length);
    return uncompressed_length;
}
} // namespace cb::compression
