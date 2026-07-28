/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <string_view>

namespace cb {

/**
 * Check whether a sequence of bytes is well-formed UTF-8.
 *
 * @param str the bytes to check
 * @return true if str is valid UTF-8, false otherwise
 */
bool is_valid_utf8(std::string_view str);

} // namespace cb
