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

#include <fmt/format.h>
#include <platform/json_log.h>
#include <platform/timeutils.h>
#include <atomic>
#include <chrono>
#include <optional>
#include <ratio>
#include <type_traits>

namespace cb::logger {

/**
 * Formats durations using cb::time2text.
 */
template <typename Rep, typename Period>
struct JsonSerializer<std::chrono::duration<Rep, Period>> {
    template <typename Json>
    static void to_json(Json& j,
                        const std::chrono::duration<Rep, Period>& val) {
        auto nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(val);
        j = cb::time2text(nanoseconds);
    }
};

/**
 * Implicit conversions for std::atomic.
 */
template <typename T>
struct JsonSerializer<std::atomic<T>> {
    template <typename Json>
    static void to_json(Json& j, const std::atomic<T>& val) {
        j = val.load(std::memory_order_relaxed);
    }
};

/**
 * Implicit conversions for std::optional.
 * std::nullopt becomes null.
 */
template <typename T>
struct JsonSerializer<std::optional<T>> {
    template <typename Json>
    static void to_json(Json& j, const std::optional<T>& val) {
        if (val) {
            j = *val;
        } else {
            j = nullptr;
        }
    }
};

namespace detail {
template <typename T, bool IsEnum>
struct use_fmt_to_string {
    static constexpr bool value = false;
};

template <typename T>
struct use_fmt_to_string<T, true> : fmt::has_formatter<T, fmt::format_context> {
};
} // namespace detail

/**
 * Conversions for enums which can be formatted using fmt::format.
 */
template <typename T>
struct JsonSerializer<
        T,
        std::enable_if_t<
                detail::use_fmt_to_string<T, std::is_enum_v<T>>::value>> {
    template <typename Json>
    static void to_json(Json& j, const T& val) {
        // Prefer the double conversion to the standard nlohmann::json, then to
        // Json, if that is possible. We have lots of types which have
        // to_json() which is not templated for different nlohmann::basic_json.
        if constexpr (std::is_constructible_v<nlohmann::json, const T&>) {
            j = nlohmann::json(val);
            return;
        }
        j = fmt::to_string(val);
    }
};

} // namespace cb::logger
