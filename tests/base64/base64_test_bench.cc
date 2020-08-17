/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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

#include <platform/base64.h>
#include <benchmark/benchmark.h>

static void BM_DecodeEmptyString(benchmark::State& state) {
    while (state.KeepRunning()) {
        cb::base64::decode("");
    }
}
BENCHMARK(BM_DecodeEmptyString);

static void BM_EncodeEmptyString(benchmark::State& state) {
    while (state.KeepRunning()) {
        cb::base64::encode({static_cast<uint8_t*>(nullptr), 0});
    }
}
BENCHMARK(BM_EncodeEmptyString);

static void BM_EncodeBlob(benchmark::State& state) {
    std::vector<uint8_t> buffer(state.range(0));
    cb::byte_buffer blob{buffer.data(), buffer.size()};

    while (state.KeepRunning()) {
        cb::base64::encode(blob);
    }
}

BENCHMARK(BM_EncodeBlob)->RangeMultiplier(100)->Range(1, 100000);

static void BM_EncodeFormattedBlob(benchmark::State& state) {
    std::vector<uint8_t> buffer(state.range(0));
    cb::byte_buffer blob{buffer.data(), buffer.size()};

    while (state.KeepRunning()) {
        cb::base64::encode(blob, true);
    }
}

BENCHMARK(BM_EncodeFormattedBlob)->RangeMultiplier(100)->Range(1, 100000);


static void BM_DecodeBlob(benchmark::State& state) {
    std::vector<uint8_t> buffer(state.range(0));
    cb::byte_buffer blob{buffer.data(), buffer.size()};
    auto input = cb::base64::encode(blob);

    while (state.KeepRunning()) {
        cb::base64::decode(input);
    }
}

BENCHMARK(BM_DecodeBlob)->RangeMultiplier(100)->Range(1, 100000);

static void BM_DecodeFormattedBlob(benchmark::State& state) {
    std::vector<uint8_t> buffer(state.range(0));
    cb::byte_buffer blob{buffer.data(), buffer.size()};
    auto input = cb::base64::encode(blob, true);

    auto idx = input.find('\n', state.range(0));
    if (idx != input.npos) {
        input.resize(idx);
    }

    while (state.KeepRunning()) {
        cb::base64::decode(input);
    }
}

BENCHMARK(BM_DecodeFormattedBlob)->RangeMultiplier(100)->Range(1, 100000);

BENCHMARK_MAIN();
