/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <optional>
#include <string>

namespace cb {
/**
 * Convert a size to a format easier for a human to interpret by convering to
 * "k", "M" etc
 *
 * @param value the value to convert
 * @param suffix n optional "suffix" to add to the string. By default adding
 *                 'b' to return sizes like: 10Mb etc
 * @return the size in a format easier to read for a human
 */
std::string size2human(std::size_t value,
                       const std::optional<std::string>& suffix = "B");

/**
 * Convert a human readable size (with an optional suffix) into a size_t.
 * Suffix may be one of: k, M, G, T, P (case insensitive) with an optional
 * 'b'/'B'
 *
 * @param text The textual representation to convert
 * @return the value in bytes
 */
std::size_t human2size(std::string_view text);
} // namespace cb
