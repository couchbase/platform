/*
 *    Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <string_view>
#include <vector>

namespace cb::string {
/**
 * Split an input string into its various parts.
 *
 * @param s The input string to split
 * @param delim The delimeter used to separate the fields
 * @param allowEmpty If false an "empty" token will not be added to the vector
 * @return A vector containing the the various tokens
 */
std::vector<std::string_view> split(std::string_view s,
                                    char delim = ' ',
                                    bool allowEmpty = true);
} // namespace cb::string
