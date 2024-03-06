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

#include <platform/json_log.h>
#include <platform/timeutils.h>
#include <atomic>
#include <chrono>
#include <ratio>

NLOHMANN_JSON_NAMESPACE_BEGIN

/**
 * Formats durations as {"text": "1h:2m:3s", "ms": 3723000}.
 */
template <typename Rep, typename Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j,
                        const std::chrono::duration<Rep, Period>& val) {
        using DoubleMilliseconds = std::chrono::duration<double, std::milli>;
        auto nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(val);
        double milliseconds =
                std::chrono::duration_cast<DoubleMilliseconds>(nanoseconds)
                        .count();
        j = {{"text", cb::time2text(nanoseconds)}, {"ms", milliseconds}};
    }
};

/**
 * Implicit conversions for std::atomic.
 */
template <typename T>
struct adl_serializer<std::atomic<T>> {
    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, const std::atomic<T>& val) {
        j = val.load(std::memory_order_relaxed);
    }
};

NLOHMANN_JSON_NAMESPACE_END
