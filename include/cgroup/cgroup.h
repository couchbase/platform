/*
 *    Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <boost/filesystem.hpp>
#include <nlohmann/json_fwd.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string_view>

namespace cb::cgroup {

/// The CPU statistics available from cgroups V1 and V2
struct CpuStat {
    std::chrono::microseconds usage{0};
    std::chrono::microseconds user{0};
    std::chrono::microseconds system{0};
    uint64_t nr_periods = 0;
    uint64_t nr_throttled = 0;
    std::chrono::microseconds throttled{0};
    uint64_t nr_bursts = 0;
    std::chrono::microseconds burst{0};
};

/// Memory information available from cgroups V1 and V2
/// Note that the information will be collected from multiple files
/// and is therefore subject to race conditions
struct MemInfo {
    MemInfo(size_t max,
            size_t current,
            size_t cache,
            size_t active_file,
            size_t inactive_file)
        : max(max),
          current(current),
          cache(cache),
          active_file(active_file),
          inactive_file(inactive_file) {
    }
    /// The max limit (if specified)
    size_t max = 0;
    /// The current (total) usage
    size_t current = 0;
    /// Current cache size
    size_t cache = 0;
    /// Current active file size
    size_t active_file = 0;
    /// Current inactive file size
    size_t inactive_file = 0;
};

/// Pressure information: See
/// https://www.kernel.org/doc/Documentation/accounting/psi.rst
enum class PressureType { Cpu, Io, Memory };
std::string to_string(PressureType type);

/// Pressure metrics collected. See
/// https://www.kernel.org/doc/Documentation/accounting/psi.rst
struct PressureMetric {
    float avg10{0};
    float avg60{0};
    float avg300{0};
    std::chrono::microseconds total_stall_time{0};
};

void to_json(nlohmann::json& json, const PressureMetric& pressure_metric);

/// The pressure data collected for each type. See
/// https://www.kernel.org/doc/Documentation/accounting/psi.rst
struct PressureData {
    PressureMetric some;
    PressureMetric full;
};

std::ostream& operator<<(std::ostream& os, const PressureType& type);
void to_json(nlohmann::json& json, const PressureData& pd);

/// The ControlGroup object offers an abstraction over cgroups v1 and v2
/// to fetch information for the cgroup where the process lives.
/// We explicitly don't want to support moving processes from one cgroup
/// to another so the cgroup mapping gets stored in a singleton (we
/// could search the various mount points to always return the information
/// from the "current" cgroup, but in a normal deployment people don't
/// move processes around). Couchbase will run all of the processes in
/// the same cgroup.
class ControlGroup {
public:
    enum class Version : uint8_t { V1 = 1, V2 = 2 };

    /// Get the CGroup version in use
    virtual Version get_version() = 0;

    virtual ~ControlGroup() = default;

    /// Get the configured number of CPUs available for this process
    /// (deprecated)
    size_t get_available_cpu_count();

    /// Get the configured number of CPUs available in % (1 full core is 100%,
    /// 1.5 cpu is 150% etc)
    size_t get_available_cpu();

    /// Get the cpu statistics
    virtual CpuStat get_cpu_stats() = 0;

    /// Get the max memory allowed for the process in the cgroup. Note that in
    /// V1 this also include the file cache etc so it is not the same as
    /// physical memory
    virtual size_t get_max_memory() = 0;

    /// Get the current memory usage by processes in the cgroup (in bytes)
    virtual size_t get_current_memory() = 0;

    /// Get the current cache size by processes in the cgroup (in bytes)
    virtual size_t get_current_cache_memory() = 0;

    /// Get the current active file by processes in the cgroup (in bytes)
    virtual size_t get_current_active_file_memory() = 0;

    /// Get the current inactive file by processes in the cgroup (in bytes)
    virtual size_t get_current_inactive_file_memory() = 0;

    /// Get the memory information
    MemInfo get_mem_info() {
        return {get_max_memory(),
                get_current_memory(),
                get_current_cache_memory(),
                get_current_active_file_memory(),
                get_current_inactive_file_memory()};
    }

    /// Get the recorded pressure data for the given type (only available
    /// on cgroup v2)
    virtual std::optional<PressureData> get_pressure_data(
            PressureType type) = 0;

    /// Get the recorded pressure data for the system
    std::optional<PressureData> get_system_pressure_data(PressureType type);

    /// Get the one and only instance used by this process
    static ControlGroup& instance();

protected:
    ControlGroup(boost::filesystem::path root) : root(std::move(root)) {
    }

    /// The root of the filesystem used to allow mocking in unit tests
    const boost::filesystem::path root;

    /// Read the cpu quota and create a CPU count
    virtual size_t get_available_cpu_count_from_quota() = 0;

    /// Parse the provided file containing the pressure information data as
    /// specified in https://www.kernel.org/doc/Documentation/accounting/psi.rst
    std::optional<PressureData> get_pressure_data_from_file(
            const boost::filesystem::path& file,
            PressureType type,
            bool global);
};

} // namespace cb::cgroup
