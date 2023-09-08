/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
 * Benchmark memory accounting - orginially a kv_engine benchmark which tested
 * EPStats memAllocated/memDeallocated, now ported to execute the JeArenaMalloc
 * tracking code.
 */

#include <benchmark/benchmark.h>
#include <platform/je_arena_malloc.h>
#include <platform/sysinfo.h>

template <class trackingImpl>
class TestJEArenaMalloc : public cb::JEArenaMalloc {
public:
    static void clientRegistered(const cb::ArenaMallocClient& client,
                                 bool arenaDebugChecksEnabled) {
        trackingImpl::clientRegistered(client, arenaDebugChecksEnabled);
    }

    static void memAllocated(uint8_t index, size_t size) {
        trackingImpl::memAllocated(index, cb::MemoryDomain::Primary, size);
    }

    static void memDeallocated(uint8_t index, size_t size) {
        trackingImpl::memDeallocated(index, cb::MemoryDomain::Primary, size);
    }
};

using BenchJEArenaMalloc = TestJEArenaMalloc<cb::JEArenaCoreLocalTracker>;

class MemoryAllocationStat : public benchmark::Fixture {
public:
    cb::ArenaMallocClient client{0, 1, true};
};

BENCHMARK_DEFINE_F(MemoryAllocationStat, AllocNRead1)(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // memUsed merge must be 4 times higher so in theory we merge at the
        // same rate as TLS (because 4 more threads than cores).
        client.estimateUpdateThreshold = 10240 * 4;
        BenchJEArenaMalloc::setAllocatedThreshold(client);
    }

    while (state.KeepRunning()) {
        // range = allocations per read
        for (int i = 0; i < state.range(0); i++) {
            if (i == 0) {
                // calling this will clear the counters
                BenchJEArenaMalloc::clientRegistered(client, false);
            } else {
                BenchJEArenaMalloc::memAllocated(1, 128);
            }
        }
        BenchJEArenaMalloc::getEstimatedAllocated(client);
    }
}

BENCHMARK_DEFINE_F(MemoryAllocationStat, AllocNReadM)(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // memUsed merge must be 4 times higher so in theory we merge at the
        // same rate as TLS (because 4 more threads than cores).
        client.estimateUpdateThreshold = 10240 * 4;
        BenchJEArenaMalloc::setAllocatedThreshold(client);
    }

    while (state.KeepRunning()) {
        // range = allocations per read
        for (int i = 0; i < state.range(0); i++) {
            if (i == 0) {
                BenchJEArenaMalloc::clientRegistered(client, false);
            } else {
                BenchJEArenaMalloc::memAllocated(1, 128);
            }
        }
        for (int j = 0; j < state.range(1); j++) {
            BenchJEArenaMalloc::getEstimatedAllocated(client);
        }
    }
}

BENCHMARK_DEFINE_F(MemoryAllocationStat, AllocNReadPreciseM)
(benchmark::State& state) {
    if (state.thread_index() == 0) {
        // memUsed merge must be 4 times higher so in theory we merge at the
        // same rate as TLS (because 4 more threads than cores).
        client.estimateUpdateThreshold = 10240 * 4;
        BenchJEArenaMalloc::setAllocatedThreshold(client);
    }

    while (state.KeepRunning()) {
        // range = allocations per read
        for (int i = 0; i < state.range(0); i++) {
            if (i == 0) {
                BenchJEArenaMalloc::clientRegistered(client, false);
            } else {
                BenchJEArenaMalloc::memAllocated(1, 128);
            }
        }
        for (int j = 0; j < state.range(1); j++) {
            BenchJEArenaMalloc::getPreciseAllocated(client);
        }
    }
}

// Tests cover a rough, but realistic range seen from a running cluster (with
// pillowfight load). The range was discovered by counting calls to
// memAllocated/deallocated and then logging how many had occurred for each
// getEstimatedTotalMemoryUsed. A previous version of this file used the Range
// API and can be used if this test is being used to perform deeper analysis of
// this code.
BENCHMARK_REGISTER_F(MemoryAllocationStat, AllocNRead1)
        ->Threads(cb::get_cpu_count() * 4)
        ->Args({0})
        ->Args({200})
        ->Args({1000});

BENCHMARK_REGISTER_F(MemoryAllocationStat, AllocNReadM)
        ->Threads(cb::get_cpu_count() * 4)
        ->Args({0, 10})
        ->Args({200, 10})
        ->Args({1000, 10})
        ->Args({0, 1000})
        ->Args({200, 200})
        ->Args({1000, 10});

BENCHMARK_REGISTER_F(MemoryAllocationStat, AllocNReadPreciseM)
        ->Threads(cb::get_cpu_count() * 4)
        // This benchmark is configured to run 'alloc heavy'. The getPrecise
        // function is only used by getStats, which is infrequent relative to
        // memory alloc/dealloc
        ->Args({1000, 10})
        ->Args({100000, 10});
