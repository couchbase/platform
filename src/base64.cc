/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <platform/base64.h>
#include <simdutf.h>
#include <stdexcept>
#include <string>

static std::string encode_impl(std::string_view source, bool uri) {
    std::string buffer;
    buffer.resize(simdutf::base64_length_from_binary(source.size()));
    auto nbutes = simdutf::binary_to_base64(
            source.data(),
            source.size(),
            buffer.data(),
            uri ? simdutf::base64_url : simdutf::base64_default);
    buffer.resize(nbutes);
    return buffer;
}

static std::string decode_impl(std::string_view blob, bool uri) {
    std::string buffer;
    buffer.resize(simdutf::maximal_binary_length_from_base64(blob.data(),
                                                             blob.size()));
    simdutf::result r = simdutf::base64_to_binary(
            blob.data(),
            blob.size(),
            buffer.data(),
            uri ? simdutf::base64_url : simdutf::base64_default,
            uri ? simdutf::loose : simdutf::strict);
    if (r.error) {
        throw std::invalid_argument("cb::base64::decode invalid input");
    }
    buffer.resize(
            r.count); // resize the buffer according to actual number of bytes
    return buffer;
}

namespace cb::base64 {
std::string encode(std::string_view view, bool) {
    return encode_impl(view, false);
}

std::string decode(std::string_view blob) {
    return decode_impl(blob, false);
}
} // namespace cb::base64

namespace cb::base64url {
std::string encode(std::string_view source) {
    return encode_impl(source, true);
}

std::string decode(std::string_view source) {
    return decode_impl(source, true);
}
} // namespace cb::base64url
