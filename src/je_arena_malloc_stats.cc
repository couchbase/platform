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

// pair<allocated, resident>
static std::pair<size_t, size_t> getFragmentation(int arena) {
    // Must request jemalloc syncs before reading stats
    callJemallocEpoch();
    std::pair<size_t, size_t> stats;
    size_t allocated;
    getJemallocStat(
            "stats.arenas." + std::to_string(arena) + ".small.allocated",
            &allocated);
    stats.first = allocated;
    getJemallocStat(
            "stats.arenas." + std::to_string(arena) + ".large.allocated",
            &allocated);
    stats.first += allocated;
    getJemallocStat("stats.arenas." + std::to_string(arena) + ".resident",
                    &stats.second);
    return stats;
}

template <>
bool cb::JEArenaMalloc::getStats(
        const cb::ArenaMallocClient& client,
        std::unordered_map<std::string, size_t>& statsMap) {
    return getJeMallocStats(client.arena, statsMap);
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
void cb::JEArenaMalloc::getDetailedStats(void (*callback)(void*, const char*),
                                         void* cbopaque) {
    je_malloc_stats_print(callback, cbopaque, "");
}

template <>
std::pair<size_t, size_t> cb::JEArenaMalloc::getFragmentationStats(
        const cb::ArenaMallocClient& client) {
    return getFragmentation(client.arena);
}

template <>
std::pair<size_t, size_t> cb::JEArenaMalloc::getGlobalFragmentationStats() {
    return getFragmentation(0);
}
