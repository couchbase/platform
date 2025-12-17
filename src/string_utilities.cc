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
#include <platform/byte_literals.h>
#include <platform/string_utilities.h>
#include <array>
#include <charconv>

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

std::size_t cb::human2size(std::string_view text) {
    size_t value = 0;

    const auto [ptr, ec]{
            std::from_chars(text.data(), text.data() + text.size(), value)};
    if (ec != std::errc()) {
        if (ec == std::errc::invalid_argument) {
            throw std::invalid_argument("human2size: no conversion");
        }
        if (ec == std::errc::result_out_of_range) {
            throw std::out_of_range("human2size: value exceeds std::size_t");
        }
        throw std::system_error(std::make_error_code(ec));
    }

    // trim off what we've parsed
    text.remove_prefix(ptr - text.data());
    if (!text.empty()) {
        switch (std::toupper(text.front())) {
        case 'P':
            value *= 1024_TiB;
            break;
        case 'T':
            value *= 1_TiB;
            break;
        case 'G':
            value *= 1_GiB;
            break;
        case 'M':
            value *= 1_MiB;
            break;
        case 'K':
            value *= 1_KiB;
            break;
        case 'B':
            break;
        default:
            throw std::invalid_argument("human2size: invalid size specifier");
        }
        text.remove_prefix(1);
        if (!text.empty()) {
            if (text.front() == 'b' || text.front() == 'B') {
                text.remove_prefix(1);
            }
            if (!text.empty()) {
                throw std::invalid_argument(
                        "human2size: Additional characters found");
            }
        }
    }

    return value;
}
