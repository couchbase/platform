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

#include <string>

namespace cb::base64 {

/**
 * Base64 encode data
 *
 * @param source the string to encode
 * @param prettyprint if set to true insert a newline after 16 chunks (4 chars)
 * @return the base64 encoded value
 */
std::string encode(std::string_view source, bool prettyprint = false);

/**
 * Decode a base64 encoded blob (which may be pretty-printed to avoid
 * super-long lines)
 *
 * @param source string to decode
 * @return the decoded data
 */
std::string decode(std::string_view blob);

} // namespace cb::base64

// For backwards source compatibility, wrap into the new
// API
namespace Couchbase::Base64 {
inline std::string encode(const std::string& source) {
    return cb::base64::encode(source, false);
}
inline std::string decode(const std::string& source) {
    return cb::base64::decode(source);
}
} // namespace Couchbase::Base64
