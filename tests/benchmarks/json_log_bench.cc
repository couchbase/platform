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
        cb::logger::Json x{{"pi", 3.141},
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
                {"pi", 3.141},
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
        nlohmann::json x{{"pi", 3.141},
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
