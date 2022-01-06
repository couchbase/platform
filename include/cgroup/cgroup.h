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

#include <chrono>
#include <cstddef>
#include <cstdint>

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

    /// Get the one and only instance used by this process
    static ControlGroup& instance();

protected:
    /// Read the cpu quota and create a CPU count
    virtual size_t get_available_cpu_count_from_quota() = 0;
};

} // namespace cb::cgroup
