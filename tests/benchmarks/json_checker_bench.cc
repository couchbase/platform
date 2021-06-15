/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <JSON_checker.h>
#include <benchmark/benchmark.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>

static void BM_JSONCheckerEmpty(benchmark::State& state) {
    JSON_checker::Validator validator;
    std::string empty = "";
    while (state.KeepRunning()) {
        validator.validate(empty);
    }
}
BENCHMARK(BM_JSONCheckerEmpty);

static void BM_NlohmannAcceptEmpty(benchmark::State& state) {
    std::string empty = "";
    while (state.KeepRunning()) {
        nlohmann::json::accept(empty);
    }
}
BENCHMARK(BM_NlohmannAcceptEmpty);

// Benchmark checking a binary object for JSON.  Object is
// "immediately" non-JSON; i.e. first byte is not a valid JSON
// starting char.
static void BM_JSONCheckerBinary(benchmark::State& state) {
    JSON_checker::Validator validator;
    const std::vector<uint8_t> binaryDoc = {1, 2, 3, 4, 5};
    // Sanity check:
    EXPECT_FALSE(validator.validate(binaryDoc));
    while (state.KeepRunning()) {
        validator.validate(binaryDoc);
    }
}
BENCHMARK(BM_JSONCheckerBinary);

static void BM_NlohmannAcceptBinary(benchmark::State& state) {
    const std::vector<uint8_t> binaryDoc = {1, 2, 3, 4, 5};
    // Sanity check:
    EXPECT_FALSE(nlohmann::json::accept(binaryDoc));
    while (state.KeepRunning()) {
        nlohmann::json::accept(binaryDoc);
    }
}
BENCHMARK(BM_NlohmannAcceptBinary);

// Benchmark checking a flat JSON array (of numbers) as JSON; e.g.
//   [ 0, 1, 2, 3, ..., N]
// Input argument 0 specifies the number of elements in the array.
std::string makeArray(benchmark::State& state) {
    std::string jsonArray = "[";
    for (int ii = 0; ii < state.range(0); ++ii) {
        jsonArray += std::to_string(ii) + ",";
    }
    // replace last comma with closing brace.
    jsonArray.back() = ']';
    return jsonArray;
}

static void BM_JSONCheckerJsonArray(benchmark::State& state) {
    auto jsonArray = makeArray(state);

    // Sanity check:
    JSON_checker::Validator validator;
    EXPECT_TRUE(validator.validate(jsonArray));

    while (state.KeepRunning()) {
        validator.validate(jsonArray);
    }
}
BENCHMARK(BM_JSONCheckerJsonArray)->RangeMultiplier(10)->Range(1, 10000);

static void BM_NlohmannAcceptJsonArray(benchmark::State& state) {
    auto jsonArray = makeArray(state);

    // Sanity check:
    EXPECT_TRUE(nlohmann::json::accept(jsonArray));

    while (state.KeepRunning()) {
        nlohmann::json::accept(jsonArray);
    }
}
BENCHMARK(BM_NlohmannAcceptJsonArray)->RangeMultiplier(10)->Range(1, 10000);

// Benchmark checking a nested JSON dictonary as JSON; e.g.
//   {"0": { "1": { ... }}}
// Input argument 0 specifies the number of levels of nesting.
std::string makeNestedDict(benchmark::State& state) {
    std::string dict;
    for (int ii = 0; ii < state.range(0); ++ii) {
        dict += "{\"" + std::to_string(ii) + "\":";
    }
    dict += "0";
    for (int ii = 0; ii < state.range(0); ++ii) {
        dict += "}";
    }
    return dict;
}

static void BM_JSONCheckerJsonNestedDict(benchmark::State& state) {
    auto dict = makeNestedDict(state);

    // Sanity check:
    JSON_checker::Validator validator;
    EXPECT_TRUE(validator.validate(dict));

    while (state.KeepRunning()) {
        EXPECT_TRUE(validator.validate(dict));
    }
}
BENCHMARK(BM_JSONCheckerJsonNestedDict)->RangeMultiplier(10)->Range(1, 10000);

static void BM_NlohmannAcceptJsonNestedDict(benchmark::State& state) {
    auto dict = makeNestedDict(state);

    // Sanity check:
    EXPECT_TRUE(nlohmann::json::accept(dict));

    while (state.KeepRunning()) {
        EXPECT_TRUE(nlohmann::json::accept(dict));
    }
}
BENCHMARK(BM_NlohmannAcceptJsonNestedDict)
        ->RangeMultiplier(10)
        ->Range(1, 10000);

// Using custom main() function (instead of BENCHMARK_MAIN macro) to
// init GoogleTest.
int main(int argc, char** argv) {
    ::testing::GTEST_FLAG(throw_on_failure) = true;
    ::testing::InitGoogleTest(&argc, argv);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
