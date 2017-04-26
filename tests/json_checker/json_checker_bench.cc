/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "config.h"

#include <JSON_checker.h>
#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

static void BM_JSONCheckerNoop(benchmark::State& state) {
    JSON_checker::Validator validator;
    while (state.KeepRunning()) {
        validator.validate(nullptr, 0);
    }
}
BENCHMARK(BM_JSONCheckerNoop);

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

// Benchmark checking a flat JSON array (of numbers) as JSON; e.g.
//   [ 0, 1, 2, 3, ..., N]
// Input argument 0 specifies the number of elements in the array.
static void BM_JSONCheckerJsonArray(benchmark::State& state) {
    JSON_checker::Validator validator;
    std::string jsonArray = "[";
    for (int ii = 0; ii < state.range(0); ++ii) {
        jsonArray += std::to_string(ii) + ",";
    }
    // replace last comma with closing brace.
    jsonArray.back() = ']';
    // Sanity check:
    EXPECT_TRUE(validator.validate(jsonArray));

    while (state.KeepRunning()) {
        validator.validate(jsonArray);
    }
}
BENCHMARK(BM_JSONCheckerJsonArray)->RangeMultiplier(10)->Range(1, 10000);

// Benchmark checking a nested JSON dictonary as JSON; e.g.
//   {"0": { "1": { ... }}}
// Input argument 0 specifies the number of levels of nesting.
static void BM_JSONCheckerJsonNestedDict(benchmark::State& state) {
    JSON_checker::Validator validator;
    std::string dict;
    for (int ii = 0; ii < state.range(0); ++ii) {
        dict += "{\"" + std::to_string(ii) + "\":";
    }
    dict += "0";
    for (int ii = 0; ii < state.range(0); ++ii) {
        dict += "}";
    }
    // Sanity check:
    EXPECT_TRUE(validator.validate(dict));

    while (state.KeepRunning()) {
        EXPECT_TRUE(validator.validate(dict));
    }
}
BENCHMARK(BM_JSONCheckerJsonNestedDict)->RangeMultiplier(10)->Range(1, 10000);


// Using custom main() function (instead of BENCHMARK_MAIN macro) to
// init GoogleTest.
int main(int argc, char** argv) {
    ::testing::GTEST_FLAG(throw_on_failure) = true;
    ::testing::InitGoogleTest(&argc, argv);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
