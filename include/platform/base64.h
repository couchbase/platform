/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <platform/sized_buffer.h>

#include <string>
#include <vector>

namespace cb::base64 {

/**
 * Base64 encode data
 *
 * @param source the string to encode
 * @return the base64 encoded value
 */
std::string encode(const cb::const_byte_buffer blob, bool prettyprint = false);

inline std::string encode(const std::string& source, bool prettyprint) {
    const_byte_buffer blob{reinterpret_cast<const uint8_t*>(source.data()),
                           source.size()};
    return encode(blob, prettyprint);
}

/**
 * Decode a base64 encoded blob (which may be pretty-printed to avoid
 * super-long lines)
 *
 * @param source string to decode
 * @return the decoded data
 */
std::vector<uint8_t> decode(std::string_view blob);

} // namespace cb::base64

// For backwards source compatibility, wrap into the new
// API
namespace Couchbase::Base64 {
/**
 * Base64 encode a string
 *
 * @param source the string to encode
 * @return the base64 encoded value
 */
inline std::string encode(const std::string& source) {
    return cb::base64::encode(source, false);
}

/**
 * Decode a base64 encoded string
 *
 * @param source string to decode
 * @return the decoded string
 */
inline std::string decode(const std::string& source) {
    auto blob = cb::base64::decode(source);
    return std::string(reinterpret_cast<const char*>(blob.data()), blob.size());
}
} // namespace Couchbase::Base64
