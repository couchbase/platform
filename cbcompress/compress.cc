/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/compression/Compression.h>
#include <folly/io/IOBuf.h>
#include <gsl/gsl-lite.hpp>
#include <platform/compress.h>
#include <snappy.h>
#include <zlib.h>

#include <stdexcept>

namespace cb::compression {
static std::unique_ptr<folly::IOBuf> deflateZlib(std::string_view input) {
    auto ret = folly::IOBuf::createCombined(
            compressBound(static_cast<uLong>(input.size())));
    uLong destlen = static_cast<uLong>(input.size()) + 128;
    const auto rv = compress(ret->writableTail(),
                             &destlen,
                             reinterpret_cast<const uint8_t*>(input.data()),
                             static_cast<uLong>(input.size()));
    if (rv != Z_OK) {
        throw std::runtime_error(fmt::format(
                "deflateZlib(): compress() failed with error code: {}", rv));
    }
    ret->append(destlen);

    return ret;
}

static std::unique_ptr<folly::IOBuf> inflateZlib(
        std::string_view input, std::size_t max_inflated_size) {
    static constexpr size_t chunk_size = 256 * 1024;
    std::unique_ptr<folly::IOBuf> ret = folly::IOBuf::create(chunk_size);

    Expects(!input.empty());

    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));

    auto status = inflateInit(&stream);
    if (status != Z_OK) {
        throw std::runtime_error(fmt::format(
                "inflateZlib(): inflateInit() failed with error code: {}",
                status));
    }

    stream.avail_in = gsl::narrow_cast<uInt>(input.size());
    stream.next_in = const_cast<uint8_t*>(
            reinterpret_cast<const uint8_t*>(input.data()));

    do {
        if (stream.total_out > max_inflated_size) {
            throw std::range_error(
                    fmt::format("inflate(): Inflated length {} "
                                " exceeds max: {}",
                                stream.total_out,
                                max_inflated_size));
        }
        auto iobuf = folly::IOBuf::create(chunk_size);
        stream.avail_out = gsl::narrow_cast<uInt>(iobuf->tailroom());
        stream.next_out = iobuf->writableTail();
        status = inflate(&stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END && status != Z_BUF_ERROR) {
            (void)inflateEnd(&stream);
            throw std::runtime_error(fmt::format(
                    "inflateZlib(): inflate() failed with error code: {}",
                    status));
        }
        iobuf->append(ret->tailroom() - stream.avail_out);
        if (!ret) {
            ret = std::move(iobuf);
        } else {
            ret->insertAfterThisOne(std::move(iobuf));
        }
    } while (status != Z_STREAM_END);
    (void)inflateEnd(&stream);
    return ret;
}

static bool inflateZlib(std::string_view input,
                        Buffer& output,
                        std::size_t max_inflated_size) {
    try {
        auto inflated = inflateZlib(input, max_inflated_size);
        output.resize(inflated->length());
        std::copy(inflated->data(),
                  inflated->data() + inflated->length(),
                  output.data());
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

static bool deflateZlib(std::string_view input, Buffer& output) {
    output.resize(compressBound(static_cast<uLong>(input.size())));
    uLong destlen = static_cast<uLong>(input.size()) + 128;
    if (compress(reinterpret_cast<uint8_t*>(output.data()),
                 &destlen,
                 reinterpret_cast<const uint8_t*>(input.data()),
                 static_cast<uLong>(input.size())) != Z_OK) {
        return false;
    }
    output.resize(destlen);

    return true;
}

bool inflate(folly::io::CodecType type,
             std::string_view input,
             Buffer& output,
             size_t max_inflated_size) {
    if (type == folly::io::CodecType::SNAPPY) {
        return inflateSnappy(input, output, max_inflated_size);
    }
    if (type == folly::io::CodecType::ZLIB) {
        return inflateZlib(input, output, max_inflated_size);
    }
    throw std::invalid_argument(
            "cb::compression::inflate(): type must be SNAPPY or ZLIB");
}

std::unique_ptr<folly::IOBuf> inflate(folly::io::CodecType type,
                                      std::string_view input,
                                      size_t max_inflated_size) {
    if (type == folly::io::CodecType::SNAPPY) {
        return inflateSnappy(input, max_inflated_size);
    }
    if (type == folly::io::CodecType::ZLIB) {
        return inflateZlib(input, max_inflated_size);
    }
    throw std::invalid_argument(
            "cb::compression::inflate(): type must be SNAPPY or ZLIB");
}

bool deflate(folly::io::CodecType type,
             std::string_view input_buffer,
             Buffer& output) {
    if (type == folly::io::CodecType::SNAPPY) {
        return deflateSnappy(input_buffer, output);
    }
    if (type == folly::io::CodecType::ZLIB) {
        return deflateZlib(input_buffer, output);
    }
    throw std::invalid_argument(
            "cb::compression::deflate(): type must be SNAPPY or ZLIB");
}

std::unique_ptr<folly::IOBuf> deflate(folly::io::CodecType type,
                                      std::string_view input) {
    if (type == folly::io::CodecType::SNAPPY) {
        return deflateSnappy(input);
    }
    if (type == folly::io::CodecType::ZLIB) {
        return deflateZlib(input);
    }
    throw std::invalid_argument(
            "cb::compression::deflate(): type must be SNAPPY or ZLIB");
}

size_t get_uncompressed_length(folly::io::CodecType type,
                               std::string_view input) {
    if (type == folly::io::CodecType::SNAPPY) {
        return getUncompressedLengthSnappy(input);
    }
    throw std::invalid_argument(
            "cb::compression::get_uncompressed_length(): type must be SNAPPY");
}

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

std::unique_ptr<folly::IOBuf> inflateSnappy(std::string_view input,
                                            size_t max_inflated_size) {
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

    auto ret = folly::IOBuf::createCombined(inflated_length);
    if (!snappy::RawUncompress(input.data(),
                               input.size(),
                               reinterpret_cast<char*>(ret->writableData()))) {
        throw std::runtime_error(
                "cb::compression::inflateSnappy: Failed to inflate data");
    }
    ret->append(inflated_length);
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

std::unique_ptr<folly::IOBuf> deflateSnappy(std::string_view input) {
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

size_t getUncompressedLengthSnappy(std::string_view input) {
    size_t uncompressed_length = 0;
    snappy::GetUncompressedLength(
            input.data(), input.size(), &uncompressed_length);
    return uncompressed_length;
}
} // namespace cb::compression
