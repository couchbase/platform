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

    throw std::invalid_argument(
            "cb::compression::to_algorithm: Unknown algorithm: " + string);
}

std::string to_string(cb::compression::Algorithm algorithm) {
    switch (algorithm) {
    case cb::compression::Algorithm::Snappy:
        return "Snappy";
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
    }
    throw std::invalid_argument(
            "cb::compression::get_uncompressed_length: Unknown compression "
            "algorithm");
}
