/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <jemalloc/jemalloc.h>
#include <platform/je_arena_malloc.h>

#include <array>
#include <cstdio>
#include <string>

// Helper function for calling mallctl
static int getJemallocStat(const std::string& property, size_t* value) {
    size_t size = sizeof(*value);
    return je_mallctl(property.c_str(), value, &size, nullptr, 0);
}

// Helper function for calling jemalloc epoch.
// jemalloc stats are not updated until the caller requests a synchronisation,
// which is done by the epoch call.
static void callJemallocEpoch() {
    size_t epoch = 1;
    size_t sz = sizeof(epoch);
    // The return of epoch is the current epoch, which we don't make use of
    (void)je_mallctl("epoch", &epoch, &sz, &epoch, sz);
}

static bool getJeMallocStats(
        int arena, std::unordered_map<std::string, size_t>& statsMap) {
    bool missing = false;
    // Must request jemalloc syncs before reading stats
    callJemallocEpoch();

    statsMap["arena"] = arena;
    std::array<std::string, 7> stats = {{"small.allocated",
                                         "large.allocated",
                                         "mapped",
                                         "retained",
                                         "internal",
                                         "base",
                                         "resident"}};
    for (auto& stat : stats) {
        std::string key = "stats.arenas." + std::to_string(arena) + "." + stat;
        size_t value{0};
        if (0 == getJemallocStat(key, &value)) {
            statsMap[stat] = value;
        } else {
            missing |= true;
        }
    }

    statsMap["allocated"] =
            statsMap["small.allocated"] + statsMap["large.allocated"];

    statsMap["fragmentation_size"] =
            statsMap["resident"] - statsMap["allocated"];
    return missing;
}

static cb::FragmentationStats getFragmentation(int arena) {
    // Must request jemalloc syncs before reading stats
    callJemallocEpoch();
    size_t allocated{0}, resident{0};
    size_t value{0};
    getJemallocStat(
            "stats.arenas." + std::to_string(arena) + ".small.allocated",
            &value);
    allocated = value;
    getJemallocStat(
            "stats.arenas." + std::to_string(arena) + ".large.allocated",
            &value);
    allocated += value;
    getJemallocStat("stats.arenas." + std::to_string(arena) + ".resident",
                    &resident);
    return {allocated, resident};
}

template <>
bool cb::JEArenaMalloc::getStats(
        const cb::ArenaMallocClient& client,
        std::unordered_map<std::string, size_t>& statsMap) {
    // TODO: Just give stats about primary domain for now, maybe aggregate ?
    return getJeMallocStats(client.arenas.at(size_t(MemoryDomain::Primary)),
                            statsMap);
}

template <>
bool cb::JEArenaMalloc::getGlobalStats(
        std::unordered_map<std::string, size_t>& statsMap) {
    return getJeMallocStats(0, statsMap);
}

struct write_state {
    cb::char_buffer buffer;
    bool cropped;
};

template <>
std::string cb::JEArenaMalloc::getDetailedStats() {
    std::string buffer;
    buffer.reserve(8192);

    auto callback = [](void* opaque, const char* msg) {
        auto& buffer = *reinterpret_cast<std::string*>(opaque);
        buffer.append(msg);
    };
    je_malloc_stats_print(callback, &buffer, "");

    return buffer;
}

template <>
cb::FragmentationStats cb::JEArenaMalloc::getFragmentationStats(
        const cb::ArenaMallocClient& client) {
    // TODO: Aggregate all arenas? For now just return primary.
    return getFragmentation(client.arenas.at(size_t(MemoryDomain::Primary)));
}

template <>
cb::FragmentationStats cb::JEArenaMalloc::getGlobalFragmentationStats() {
    return getFragmentation(0);
}
