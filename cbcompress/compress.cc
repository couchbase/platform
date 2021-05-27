/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <gsl/gsl-lite.hpp>
#include <platform/cb_malloc.h>
#include <platform/compress.h>
#include <snappy.h>
#include <cctype>
#include <stdexcept>

#ifdef CB_LZ4_SUPPORT
#include <lz4.h>
#endif


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

static bool doLZ4Uncompress(std::string_view input,
                            cb::compression::Buffer& output,
                            size_t max_inflated_size) {
#ifdef CB_LZ4_SUPPORT
    if (input.size() < 4) {
        // The length of the compressed data is stored in the first 4 bytes
        // in network byte order
        return false;
    }

    size_t size = ntohl(*reinterpret_cast<const uint32_t*>(input.data()));
    if (size > max_inflated_size) {
        return false;
    }

    output.resize(size);
    auto nb = LZ4_decompress_safe(input.data() + 4,
                                  output.data(),
                                  gsl::narrow_cast<int>(input.size() - 4),
                                  gsl::narrow_cast<int>(size));

    return nb == gsl::narrow_cast<int>(size);

#else
    throw std::runtime_error("doLZ4Uncompress: LZ4 not supported");
#endif
}

static size_t doLZ4UncompressedLength(std::string_view buffer) {
#ifdef CB_LZ4_SUPPORT
    return ntohl(*reinterpret_cast<const uint32_t*>(buffer.data()));
#else
    throw std::runtime_error("doLZ4UncompressedLength: LZ4 not supported");
#endif
}

static bool doLZ4Compress(std::string_view input,
                          cb::compression::Buffer& output) {
#ifdef CB_LZ4_SUPPORT
    const auto buffersize =
            size_t(LZ4_compressBound(gsl::narrow_cast<int>(input.size())));
    output.resize(buffersize + 4);
    // The length of the compressed data is stored in the first 4 bytes
    // in network byte order
    auto* size_ptr = reinterpret_cast<uint32_t*>(output.data());
    *size_ptr = htonl(gsl::narrow_cast<uint32_t>(input.size()));

    auto compressed_length =
            LZ4_compress_default(input.data(),
                                 output.data() + 4,
                                 gsl::narrow_cast<int>(input.size()),
                                 gsl::narrow_cast<int>(buffersize));

    if (compressed_length <= 0) {
        return false;
    }

    // Include the length bytes..
    output.resize(size_t(compressed_length + 4));
    return true;
#else
    throw std::runtime_error("doLZ4Compress: LZ4 not supported");
#endif
}

bool cb::compression::inflate(Algorithm algorithm,
                              std::string_view input_buffer,
                              Buffer& output,
                              size_t max_inflated_size) {
    switch (algorithm) {
    case Algorithm::Snappy:
        if (!doSnappyUncompress(input_buffer, output, max_inflated_size)) {
            output.reset();
            return false;
        }
        return true;
    case Algorithm::LZ4:
        if (!doLZ4Uncompress(input_buffer, output, max_inflated_size)) {
            output.reset();
            return false;
        }
        return true;
    }
    throw std::invalid_argument(
        "cb::compression::inflate: Unknown compression algorithm");
}

bool cb::compression::deflate(Algorithm algorithm,
                              std::string_view input_buffer,
                              Buffer& output) {
    switch (algorithm) {
    case Algorithm::Snappy:
        if (!doSnappyCompress(input_buffer, output)) {
            output.reset();
            return false;
        }
        return true;
    case Algorithm::LZ4:
        if (!doLZ4Compress(input_buffer, output)) {
            output.reset();
            return false;
        }
        return true;
    }
    throw std::invalid_argument(
        "cb::compression::deflate: Unknown compression algorithm");
}

cb::compression::Algorithm cb::compression::to_algorithm(
        const std::string& string) {
    std::string input;
    std::transform(
            string.begin(), string.end(), std::back_inserter(input), ::toupper);

    if (input == "SNAPPY") {
        return Algorithm::Snappy;
    }

    if (input == "LZ4") {
        return Algorithm::LZ4;
    }

    throw std::invalid_argument(
            "cb::compression::to_algorithm: Unknown algorithm: " + string);
}

std::string to_string(cb::compression::Algorithm algorithm) {
    switch (algorithm) {
    case cb::compression::Algorithm::Snappy:
        return "Snappy";
    case cb::compression::Algorithm::LZ4:
        return "LZ4";
    }

    throw std::invalid_argument(
            "to_string(cb::compression::Algorithm): Unknown compression "
            "algorithm");
}

bool cb::compression::validate(cb::compression::Algorithm algorithm,
                               std::string_view input_buffer,
                               size_t max_inflated_size) {
    switch (algorithm) {
    case Algorithm::Snappy:
        return doSnappyValidate(input_buffer);
    case Algorithm::LZ4:
        cb::compression::Buffer output;
        return doLZ4Uncompress(input_buffer, output, max_inflated_size);
    }
    throw std::invalid_argument(
        "cb::compression::validate: Unknown compression algorithm");
}

size_t cb::compression::get_uncompressed_length(
        cb::compression::Algorithm algorithm,
        std::string_view input_buffer) {
    switch (algorithm) {
    case Algorithm::Snappy:
        return doSnappyUncompressedLength(input_buffer);
    case Algorithm::LZ4:
        return doLZ4UncompressedLength(input_buffer);
    }
    throw std::invalid_argument(
            "cb::compression::get_uncompressed_length: Unknown compression "
            "algorithm");
}
