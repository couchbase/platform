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

#include <platform/compress.h>

#include <snappy.h>
#include <stdexcept>

static bool doSnappyUncompress(cb::const_char_buffer input,
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

    std::unique_ptr<char[]> temp(new char[inflated_length]);
    if (!snappy::RawUncompress(input.data(), input.size(), temp.get())) {
        return false;
    }

    output.data = std::move(temp);
    output.len = inflated_length;
    return true;
}

static bool doSnappyCompress(cb::const_char_buffer input,
                             cb::compression::Buffer& output) {
    size_t compressed_length = snappy::MaxCompressedLength(input.size());
    std::unique_ptr<char[]> temp(new char[compressed_length]);
    snappy::RawCompress(
            input.data(), input.size(), temp.get(), &compressed_length);
    output.data = std::move(temp);
    output.len = compressed_length;
    return true;
}

bool cb::compression::inflate(Algorithm algorithm,
                              cb::const_char_buffer input_buffer,
                              Buffer& output,
                              size_t max_inflated_size) {
    switch (algorithm) {
    case Algorithm::Snappy:
        return doSnappyUncompress(input_buffer, output, max_inflated_size);
    }
    throw std::invalid_argument(
        "cb::compression::inflate: Unknown compression algorithm");
}

bool cb::compression::deflate(Algorithm algorithm,
                              cb::const_char_buffer input_buffer,
                              Buffer& output) {
    switch (algorithm) {
    case Algorithm::Snappy:
        return doSnappyCompress(input_buffer, output);
    }
    throw std::invalid_argument(
        "cb::compression::deflate: Unknown compression algorithm");
}
