/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "cgroup_private.h"

#include <cgroup/cgroup.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <system_error>

namespace cb::cgroup {

void to_json(nlohmann::json& json, const PressureMetric& pressure_metric) {
    json = {{"avg10", pressure_metric.avg10},
            {"avg60", pressure_metric.avg60},
            {"avg300", pressure_metric.avg300},
            {"total_stall_time_usec",
             std::to_string(pressure_metric.total_stall_time.count())}};
}

void to_json(nlohmann::json& json, const PressureData& pd) {
    json = {{"some", pd.some}, {"full", pd.full}};
}

std::string to_string(PressureType type) {
    switch (type) {
    case PressureType::Cpu:
        return "cpu";
    case PressureType::Io:
        return "io";
    case PressureType::Memory:
        return "memory";
    }
    throw std::invalid_argument("to_string() Invalid PressureType: " +
                                std::to_string(int(type)));
}

std::ostream& operator<<(std::ostream& os, const PressureType& type) {
    os << to_string(type);
    return os;
}

size_t ControlGroup::get_available_cpu_count() {
    auto ret = get_available_cpu();
    auto cpu = ret / 100;
    if (ret % 100) {
        ++cpu;
    }
    return cpu;
}

size_t ControlGroup::get_available_cpu() {
    const auto num = get_available_cpu_count_from_quota();
    if (num != 0) {
        return num;
    }

#ifdef __linux__
    // The library is only intended to be used on Linux in production,
    // but is also built on mac to make the life easier for developers
    // (as the unit tests operate on files in the repo). cpu_set_t isn't
    // available on macosx
    cpu_set_t set;
    if (sched_getaffinity(getpid(), sizeof(set), &set) == 0) {
        return CPU_COUNT(&set) * 100;
    }
#endif

    auto ret = sysconf(_SC_NPROCESSORS_ONLN);
    if (ret == -1) {
        throw std::system_error(
                std::error_code(errno, std::system_category()),
                "cb::cgroup::get_available_cpu_count(): sysconf failed");
    }

    return size_t(ret) * 100;
}

ControlGroup& ControlGroup::instance() {
    static auto instance = priv::make_control_group();
    return *instance;
}

void ControlGroup::setTraceCallback(std::function<void(std::string_view)> cb) {
    priv::setTraceCallback(std::move(cb));
}

std::optional<PressureData> ControlGroup::get_pressure_data_from_file(
        const std::filesystem::path& file, PressureType type, bool global) {
    if (!exists(file)) {
        return {};
    }

    PressureData data;
    bool some = false;
    bool full = false;

    cb::io::tokenizeFileLineByLine(
            file, [&data, &some, &full](const auto& parts) {
                if (parts.size() < 5) {
                    return true;
                }

                if ((parts.front() != "some" && parts.front() != "full") ||
                    parts[1].find("avg10=") != 0 ||
                    parts[2].find("avg60=") != 0 ||
                    parts[3].find("avg300=") != 0 ||
                    parts[4].find("total=") != 0) {
                    // unexpected line
                    return true;
                }
                try {
                    PressureMetric metric;
                    metric.avg10 = std::stof(std::string(parts[1].substr(6)));
                    metric.avg60 = std::stof(std::string(parts[2].substr(6)));
                    metric.avg300 = std::stof(std::string(parts[3].substr(7)));
                    metric.total_stall_time = std::chrono::microseconds{
                            std::stoull(std::string(parts[4].substr(6)))};

                    if (parts.front() == "some") {
                        data.some = metric;
                        some = true;
                    } else {
                        data.full = metric;
                        full = true;
                    }
                } catch (const std::exception&) {
                    // swallow
                }
                return true;
            });

    if (some && full) {
        return data;
    }

    // Ubuntu 20.04 does not provide a full line for the global CPU pressure
    if (some && !full && type == PressureType::Cpu && global) {
        data.full = {};
        return data;
    }

    return {};
}

/// Get the recorded pressure data for the system
std::optional<PressureData> ControlGroup::get_system_pressure_data(
        PressureType type) {
    switch (type) {
    case PressureType::Cpu:
        return get_pressure_data_from_file(
                root / "proc" / "pressure" / "cpu", PressureType::Cpu, true);
    case PressureType::Io:
        return get_pressure_data_from_file(
                root / "proc" / "pressure" / "io", PressureType::Io, true);
    case PressureType::Memory:
        return get_pressure_data_from_file(
                root / "proc" / "pressure" / "memory",
                PressureType::Memory,
                true);
    }
    throw std::invalid_argument("get_system_pressure_data(): Unknown type: " +
                                std::to_string(int(type)));
}

} // namespace cb::cgroup
