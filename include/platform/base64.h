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
 * @return the base64 encoded value
 */
std::string encode(std::string_view source);

/**
 * Decode a base64 encoded blob (which may be pretty-printed to avoid
 * super-long lines)
 *
 * @param blob string to decode
 * @return the decoded data
 */
std::string decode(std::string_view blob);

} // namespace cb::base64

/**
 * An alternative encoding using '-' and '_' for the 62 and 63rd character
 * in the alphabet.
 */
namespace cb::base64url {
std::string encode(std::string_view source);
std::string decode(std::string_view source);
} // namespace cb::base64url
