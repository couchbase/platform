/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
        cb::base64::encode({});
    }
}
BENCHMARK(BM_EncodeEmptyString);

static void BM_EncodeBlob(benchmark::State& state) {
    std::string buffer;
    buffer.resize(state.range(0));
    while (state.KeepRunning()) {
        cb::base64::encode(buffer);
    }
}

BENCHMARK(BM_EncodeBlob)->RangeMultiplier(100)->Range(1, 100000);

static void BM_EncodeFormattedBlob(benchmark::State& state) {
    std::string buffer;
    buffer.resize(state.range(0));
    while (state.KeepRunning()) {
        cb::base64::encode(buffer, true);
    }
}

BENCHMARK(BM_EncodeFormattedBlob)->RangeMultiplier(100)->Range(1, 100000);


static void BM_DecodeBlob(benchmark::State& state) {
    std::string buffer;
    buffer.resize(state.range(0));
    auto input = cb::base64::encode(buffer);

    while (state.KeepRunning()) {
        cb::base64::decode(input);
    }
}

BENCHMARK(BM_DecodeBlob)->RangeMultiplier(100)->Range(1, 100000);

static void BM_DecodeFormattedBlob(benchmark::State& state) {
    std::string buffer;
    buffer.resize(state.range(0));
    auto input = cb::base64::encode(buffer, true);

    auto idx = input.find('\n', state.range(0));
    if (idx != input.npos) {
        input.resize(idx);
    }

    while (state.KeepRunning()) {
        cb::base64::decode(input);
    }
}

BENCHMARK(BM_DecodeFormattedBlob)->RangeMultiplier(100)->Range(1, 100000);
