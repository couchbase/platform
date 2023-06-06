/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <benchmark/benchmark.h>

#include <snappy.h>
#include <array>
#include <memory>

// We don't use the cbcompress API here, as it include memory allocation
// in each operation..
#define START 256
#define END 40960
#define FACTOR 2

std::array<char, END> blob;

static void SnappyCompress(benchmark::State& state) {
    const auto size = size_t(state.range(0));
    size_t compressed_length = snappy::MaxCompressedLength(size);
    std::unique_ptr<char[]> temp(new char[compressed_length]);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(temp.get());
        snappy::RawCompress(blob.data(), size, temp.get(), &compressed_length);
#ifndef WIN32
        benchmark::ClobberMemory();
#endif
    }
    state.counters["compressed"] = compressed_length;
}

BENCHMARK(SnappyCompress)->RangeMultiplier(FACTOR)->Range(START, END);

int main(int argc, char** argv) {
    int ii = 0;
    for (auto& a : blob) {
        a = 'a' + (ii++ % ('z' - 'a'));
    }

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
