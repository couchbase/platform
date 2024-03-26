/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <fmt/format.h>
#include <platform/string_utilities.h>
#include <array>

std::string cb::size2human(std::size_t value,
                           const std::optional<std::string>& suffix) {
    const std::array<const char*, 6> size_suffix{{"", "k", "M", "G", "T", "P"}};
    std::size_t index = 0;
    while (value > 10240 && index < (size_suffix.size() - 1)) {
        value /= 1024;
        ++index;
    }
    return fmt::format("{}{}{}",
                       value,
                       size_suffix[index],
                       suffix.has_value() ? *suffix : "");
}
