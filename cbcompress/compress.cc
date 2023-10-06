/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/compress.h>
#include <snappy.h>
#include <cctype>
#include <stdexcept>

static bool doSnappyUncompress(std::string_view input,
                               cb::compression::Buffer& output,
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

static bool doSnappyCompress(std::string_view input,
                             cb::compression::Buffer& output) {
    size_t compressed_length = snappy::MaxCompressedLength(input.size());
    output.resize(compressed_length);
    snappy::RawCompress(
            input.data(), input.size(), output.data(), &compressed_length);
    output.resize(compressed_length);
    return true;
}

static bool doSnappyValidate(std::string_view buffer) {
    return snappy::IsValidCompressedBuffer(buffer.data(), buffer.size());
}

static size_t doSnappyUncompressedLength(std::string_view buffer) {
    size_t uncompressed_length = 0;
    snappy::GetUncompressedLength(
            buffer.data(), buffer.size(), &uncompressed_length);
    return uncompressed_length;
}

bool cb::compression::inflate(folly::io::CodecType,
                              std::string_view input_buffer,
                              Buffer& output,
                              size_t max_inflated_size) {
    if (!doSnappyUncompress(input_buffer, output, max_inflated_size)) {
        output.reset();
        return false;
    }
    return true;
}

std::unique_ptr<folly::IOBuf> cb::compression::inflate(
        folly::io::CodecType type,
        std::string_view input,
        size_t max_inflated_size) {
    if (type != folly::io::CodecType::SNAPPY) {
        throw std::invalid_argument(
                "cb::compression::inflate(): type must be SNAPPY");
    }

    size_t inflated_length;
    if (!snappy::GetUncompressedLength(
                input.data(), input.size(), &inflated_length)) {
        throw std::runtime_error(
                "cb::compression::inflate(Snappy): Failed to get uncompressed "
                "length");
    }

    if (inflated_length > max_inflated_size) {
        throw std::range_error(
                fmt::format("cb::compression::inflate(): Inflated length {} "
                            "would exceed max: {}",
                            inflated_length,
                            max_inflated_size));
    }

    auto ret = folly::IOBuf::createCombined(inflated_length);
    if (!snappy::RawUncompress(input.data(),
                               input.size(),
                               reinterpret_cast<char*>(ret->writableData()))) {
        throw std::runtime_error(
                "cb::compression::inflate(Snappy): Failed to inflate data");
    }
    ret->append(inflated_length);
    return ret;
}

bool cb::compression::deflate(folly::io::CodecType,
                              std::string_view input_buffer,
                              Buffer& output) {
    if (!doSnappyCompress(input_buffer, output)) {
        output.reset();
        return false;
    }
    return true;
}

std::unique_ptr<folly::IOBuf> cb::compression::deflate(
        folly::io::CodecType type, std::string_view input) {
    if (type != folly::io::CodecType::SNAPPY) {
        throw std::invalid_argument(
                "cb::compression::deflate(): type must be SNAPPY");
    }

    size_t max_compressed_length = snappy::MaxCompressedLength(input.size());
    auto ret = folly::IOBuf::createCombined(max_compressed_length);
    size_t compressed_length = max_compressed_length;
    snappy::RawCompress(input.data(),
                        input.size(),
                        reinterpret_cast<char*>(ret->writableData()),
                        &compressed_length);
    ret->append(compressed_length);
    return ret;
}

bool cb::compression::validate(folly::io::CodecType,
                               std::string_view input_buffer,
                               size_t max_inflated_size) {
    return doSnappyValidate(input_buffer);
}

size_t cb::compression::get_uncompressed_length(folly::io::CodecType,
                                                std::string_view input_buffer) {
    return doSnappyUncompressedLength(input_buffer);
}

size_t cb::compression::getUncompressedLengthSnappy(
        std::string_view input_buffer) {
    return doSnappyUncompressedLength(input_buffer);
}
