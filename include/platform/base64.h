/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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
#pragma once

#include <platform/sized_buffer.h>
#include <platform/visibility.h>

#include <string>
#include <vector>

namespace cb {
namespace base64 {

/**
 * Base64 encode data
 *
 * @param source the string to encode
 * @return the base64 encoded value
 */
PLATFORM_PUBLIC_API
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
PLATFORM_PUBLIC_API
std::vector<uint8_t> decode(const cb::const_char_buffer blob);

} // namespace base64
} // namespace cb

// For backwards source compatibility, wrap into the new
// API
namespace Couchbase {
namespace Base64 {
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
} // namespace Base64
} // namespace Couchbase
