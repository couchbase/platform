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

#include <fmt/core.h>
#include <gsl/gsl-lite.hpp>
#include <nlohmann/json.hpp>
#include <platform/cb_allocator.h>
#include <platform/ordered_map.h>

namespace cb::logger {
namespace detail {

/**
 * The allocator used for JSON log object. They are always allocated from the
 * global arena. This makes it easier to reason about the memory usage of the
 * JSON log messages while avoiding the need for defensive copying.
 */
template <typename T>
struct Allocator : cb::NoArenaAllocator<T> {};

/**
 * Like std::string, but using our allocator. Note that constructing from
 * std::string always requires a copy (even from std::string&&).
 */
using String = std::
        basic_string<char, std::char_traits<char>, detail::Allocator<char>>;

/**
 * Map type used for objects stored in a LogJson object.
 */
template <typename Key, typename T, typename KeyCompare, typename Allocator>
using Map = cb::OrderedMap<
        Key,
        T,
        KeyCompare,
        std::vector<std::pair<Key, T>,
                    typename std::allocator_traits<Allocator>::
                            template rebind_alloc<std::pair<Key, T>>>>;

} // namespace detail

template <typename T, typename SFINAE = void>
struct JsonSerializer : nlohmann::adl_serializer<T, SFINAE> {};

/**
 * Custom JSON type used for logging with the following properties:
 * - uses cb::OrderedMap to preserve ordering of JSON keys
 * - uses cb::NoArenaAllocator for all internal allocations
 *
 * Can be used in the signature of:
 *   void to_json(Json j, T value);
 * To customize how objects are represented in log messages.
 */
using Json = nlohmann::basic_json<
        detail::Map,
        std::vector,
        detail::String,
        bool,
        std::int64_t,
        std::uint64_t,
        double,
        detail::Allocator,
        JsonSerializer,
        std::vector<std::uint8_t, detail::Allocator<std::uint8_t>>>;

} // namespace cb::logger

template <>
struct fmt::formatter<cb::logger::Json> : formatter<string_view> {
    auto format(cb::logger::Json json, format_context& ctx) const {
        // Non-ASCII characters are escaped as \uXXXX
        const auto ensure_ascii = true;
        // Replaces invalid sequences with \ufffd (U+FFFD)
        const auto error_handler = nlohmann::json::error_handler_t::replace;
        return fmt::format_to(ctx.out(),
                              "{}",
                              json.dump(-1, ' ', ensure_ascii, error_handler));
    }
};

// Automatic conversions do not need to be included separately.
#include <platform/json_log_conversions.h>
