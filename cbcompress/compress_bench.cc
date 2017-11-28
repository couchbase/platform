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

#include <benchmark/benchmark.h>

#include <snappy.h>
#include <array>
#include <memory>

#ifdef CB_LZ4_SUPPORT
#include <lz4.h>
#endif

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

#ifdef CB_LZ4_SUPPORT
static void Lz4Compress(benchmark::State& state) {
    const auto size = size_t(state.range(0));
    const auto buffersize = size_t(LZ4_compressBound(state.range(0)));
    std::unique_ptr<char[]> temp(new char[buffersize]);
    int compressed_length = 0;

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(temp.get());
        compressed_length = LZ4_compress_default(
                blob.data(), temp.get(), int(size), int(buffersize));
        if (compressed_length <= 0) {
            abort();
        }
#ifndef WIN32
        benchmark::ClobberMemory();
#endif
    }

    state.counters["compressed"] = compressed_length;
}

BENCHMARK(Lz4Compress)->RangeMultiplier(FACTOR)->Range(START, END);
#endif

int main(int argc, char** argv) {
    int ii = 0;
    for (auto& a : blob) {
        a = 'a' + (ii++ % ('z' - 'a'));
    }

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
