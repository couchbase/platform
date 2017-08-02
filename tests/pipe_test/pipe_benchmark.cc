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
#include <platform/make_unique.h>
#include <platform/pipe.h>
#include <algorithm>

// Benchmark copying data into a blob. This represents how we used to add
// to the old write buffer
void PlainMemcpy(benchmark::State& state) {
    std::vector<uint8_t> blob(256);
    std::vector<uint8_t> data(4096);

    while (state.KeepRunning()) {
        std::copy(blob.begin(), blob.end(), data.begin());
    }
}
BENCHMARK(PlainMemcpy);

// Benchmark calling the produce part of the pipe to insert data into the
// send buffer.
void Produce(benchmark::State& state) {
    std::vector<uint8_t> blob(256);
    cb::Pipe pipe(4096);

    while (state.KeepRunning()) {
        pipe.clear();
        pipe.produce([&blob](void* ptr, size_t size) -> size_t {
            std::copy(blob.begin(), blob.end(), static_cast<uint8_t*>(ptr));
            return blob.size();
        });
    }
}
BENCHMARK(Produce);

// Benchmark calling the consume part of the pipe to check the data just
// being sent points to the buffer (this represents the action we've added
// to see if just sent data
void Consume(benchmark::State& state) {
    std::vector<uint8_t> blob(256);
    cb::Pipe pipe(4096);
    pipe.produce([](void*, size_t) -> size_t { return 4; });
    while (state.KeepRunning()) {
        const char* nil = nullptr;
        pipe.consume([&nil](const void* ptr, size_t size) -> size_t {
            if (nil == ptr) {
                return 0;
            }
            return 0;
        });
    }
}
BENCHMARK(Consume);

// Benchmark fetching the read end of the pipe to peek at the
// data
void Rdata(benchmark::State& state) {
    cb::Pipe pipe(4096);
    pipe.produce([](void*, size_t) -> size_t { return 4; });
    while (state.KeepRunning()) {
        pipe.rdata();
    }
}
BENCHMARK(Rdata);


BENCHMARK_MAIN()
