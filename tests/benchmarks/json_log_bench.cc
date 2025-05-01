/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <benchmark/benchmark.h>
#include <fmt/format.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/json_log.h>

static void BM_Logger_LogJson(benchmark::State& state) {
    while (state.KeepRunning()) {
        cb::logger::Json x{{"pi", 3.14},
                           {"happy", true},
                           {"name", "Niels"},
                           {"nothing", nullptr},
                           {"answer", {{"everything", 42}}},
                           {"list", {1, 0, 2}},
                           {"object", {{"currency", "USD"}, {"value", 42.99}}}};
        (void)x.dump();
    }
}
BENCHMARK(BM_Logger_LogJson);

static void BM_Logger_NlohmannOrdered(benchmark::State& state) {
    while (state.KeepRunning()) {
        nlohmann::ordered_json x{
                {"pi", 3.14},
                {"happy", true},
                {"name", "Niels"},
                {"nothing", nullptr},
                {"answer", {{"everything", 42}}},
                {"list", {1, 0, 2}},
                {"object", {{"currency", "USD"}, {"value", 42.99}}}};
        (void)x.dump();
    }
}
BENCHMARK(BM_Logger_NlohmannOrdered);

static void BM_Logger_NlohmannJson(benchmark::State& state) {
    while (state.KeepRunning()) {
        nlohmann::json x{{"pi", 3.14},
                         {"happy", true},
                         {"name", "Niels"},
                         {"nothing", nullptr},
                         {"answer", {{"everything", 42}}},
                         {"list", {1, 0, 2}},
                         {"object", {{"currency", "USD"}, {"value", 42.99}}}};
        (void)x.dump();
    }
}
BENCHMARK(BM_Logger_NlohmannJson);

// Similar to what we do in KV to merge the prefix with the context.
template <typename JsonType>
void mergeContext(JsonType& dest, JsonType src) {
    auto& destObj = dest.template get_ref<typename JsonType::object_t&>();
    auto& srcObj = src.template get_ref<typename JsonType::object_t&>();
    std::move(srcObj.begin(), srcObj.end(), std::back_inserter(destObj));
}

template <typename JsonType>
void dumpWithPrefix(const nlohmann::ordered_json& prefix, JsonType ctx) {
    auto finalCtx = JsonType(prefix);
    mergeContext(finalCtx, std::move(ctx));
    (void)finalCtx.dump();
}

// Test merging with our own JsonType.
static void BM_Logger_Merge_LogJson(benchmark::State& state) {
    const nlohmann::ordered_json prefix{
            {"pi", 3.14}, {"happy", true}, {"name", "Niels"}};
    while (state.KeepRunning()) {
        dumpWithPrefix(
                prefix,
                cb::logger::BasicJsonType{
                        {"nothing", nullptr},
                        {"answer", {{"everything", 42}}},
                        {"list", {1, 0, 2}},
                        {"object", {{"currency", "USD"}, {"value", 42.99}}}});
    }
}
BENCHMARK(BM_Logger_Merge_LogJson);

// Test merging with nlohmann::ordered_json.
static void BM_Logger_Merge_NlohmannOrdered(benchmark::State& state) {
    const nlohmann::ordered_json prefix{
            {"pi", 3.14}, {"happy", true}, {"name", "Niels"}};
    while (state.KeepRunning()) {
        dumpWithPrefix(
                prefix,
                nlohmann::ordered_json{
                        {"nothing", nullptr},
                        {"answer", {{"everything", 42}}},
                        {"list", {1, 0, 2}},
                        {"object", {{"currency", "USD"}, {"value", 42.99}}}});
    }
}
BENCHMARK(BM_Logger_Merge_NlohmannOrdered);
