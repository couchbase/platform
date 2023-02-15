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
#include <fmt/core.h>
#include <platform/dirutils.h>
#include <platform/split_string.h>
#include <unistd.h>
#include <charconv>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace std::string_view_literals;

namespace cb::cgroup::priv {

static std::function<void(std::string_view)> traceCallback;

void setTraceCallback(std::function<void(std::string_view)> cb) {
    traceCallback = std::move(cb);
}

uint64_t stouint64(std::string_view view) {
    uint64_t ret;
    auto [ptr, ec] = std::from_chars(view.begin(), view.end(), ret);
    if (ec == std::errc()) {
        return ret;
    }
    (void)ptr;
    return 0;
}

struct MountEntry {
    enum class Version { V1, V2 };
    MountEntry(std::string_view t, const std::string& p, std::string o)
        : type(t == "cgroup2" ? Version::V2 : Version::V1),
          path(p),
          option(std::move(o)) {
    }
    const Version type;
    const std::filesystem::path path;
    const std::string option;
};

/// Read out the cgroups entries from /proc/mounts
/// throws std::system_error on errors
static std::vector<MountEntry> parse_proc_mounts(const std::string root) {
    std::vector<MountEntry> ret;
    if (traceCallback) {
        traceCallback(fmt::format("Parsing {}/proc/mounts", root));
    }
    cb::io::tokenizeFileLineByLine(
            root + "/proc/mounts", [&ret, &root](const auto& parts) {
                // pick out the lines which looks like:
                //   [something] /sys/fs/cgroup cgroup[2] rw,nosuid,optionblah
                if (parts.size() > 3 &&
                    parts[2].find("cgroup") != std::string::npos) {
                    if (traceCallback) {
                        traceCallback(fmt::format("Using entry {} {} {}",
                                                  parts[2],
                                                  parts[1],
                                                  parts[3]));
                    }
                    ret.emplace_back(MountEntry{std::string(parts[2]),
                                                root + std::string(parts[1]),
                                                std::string(parts[3])});
                }
                return true;
            });
    return ret;
}

static bool search_file(pid_t pid, const std::filesystem::path& file) {
    bool found = false;
    const auto textual = std::to_string(pid);
    cb::io::tokenizeFileLineByLine(file, [&found, &textual](const auto& parts) {
        if (!parts.empty() && parts.front() == textual) {
            found = true;
        }
        return true;
    });
    return found;
}

/**
 * Search the subtree from root for all the cgroup.procs files where the
 * current process is located.
 *
 * @param root the root directory to search in
 * @param pid the pid to search for
 * @return the path to where I found the pid registered
 * @thros std::system_error for io errors
 */
static std::optional<std::filesystem::path> find_cgroup_path(
        const std::filesystem::path& root, pid_t pid) {
    if (traceCallback) {
        traceCallback(fmt::format("Try to locate my pid ({}) in in {}",
                                  pid,
                                  root.generic_string()));
    }

    std::vector<std::filesystem::path> ret;
    std::deque<std::filesystem::path> paths;
    paths.push_back(root);
    while (!paths.empty()) {
        auto path = paths.front();
        paths.pop_front();

        if (exists(path)) {
            auto file = path / "cgroup.procs";
            if (exists(file) && search_file(pid, file)) {
                if (traceCallback) {
                    traceCallback(fmt::format("Found it in {}",
                                              file.generic_string()));
                }
                return path;
            }

            for (const auto& p : std::filesystem::directory_iterator(path)) {
                if (is_directory(p) && !is_symlink(p)) {
                    paths.push_back(p.path());
                }
            }
        }
    }

    if (traceCallback) {
        traceCallback("Pid not found anywhere"sv);
    }
    return {};
}

class ControlGroupV1 : public ControlGroup {
public:
    ControlGroupV1(std::filesystem::path root) : ControlGroup(root) {
        user_hz = sysconf(_SC_CLK_TCK);
        if (user_hz == -1) {
            if (traceCallback) {
                traceCallback(
                        "sysconf(_SC_CLK_TCK) returned -1. Default to 100"sv);
            }
            user_hz = 100;
        }
    }

    void add_entry(const std::filesystem::path& path,
                   std::string_view options) {
        auto tokens = cb::string::split(options, ',');
        for (const auto& token : tokens) {
            if (token == "cpu") {
                if (traceCallback) {
                    traceCallback(fmt::format("Adding cpu controller from {}",

                                              path.generic_string()));
                }
                cpu = path;
            }
            if (token == "cpuacct") {
                if (traceCallback) {
                    traceCallback(
                            fmt::format("Adding cpuacct controller from {}",
                                        path.generic_string()));
                }
                cpuacct = path;
            }
            if (token == "memory") {
                if (traceCallback) {
                    traceCallback(
                            fmt::format("Adding memory controller from {}",
                                        path.generic_string()));
                }
                memory = path;
            }
        }
    }

    Version get_version() override {
        return Version::V1;
    }

    CpuStat get_cpu_stats() override {
        CpuStat stats;

        if (cpuacct) {
            auto fname = *cpuacct / "cpuacct.stat";
            if (exists(fname)) {
                if (traceCallback) {
                    traceCallback(fmt::format("Try to read {}",
                                              fname.generic_string()));
                }
                cb::io::tokenizeFileLineByLine(
                        fname, [&stats, this](const auto& parts) {
                            if (parts.size() < 2) {
                                return true;
                            }

                            // CPU time is reported in the units defined by
                            // the USER_HZ variable (for some weird reason).
                            // This value is typically set to 100
                            if (parts.front() == "user") {
                                stats.user = std::chrono::microseconds{
                                        stouint64(parts[1]) * (1000 / user_hz) *
                                        1000};
                            } else if (parts.front() == "system") {
                                stats.system = std::chrono::microseconds{
                                        stouint64(parts[1]) * (1000 / user_hz) *
                                        1000};
                            }
                            return true;
                        });
            }
            fname = *cpuacct / "cpuacct.usage";
            if (exists(fname)) {
                if (traceCallback) {
                    traceCallback(fmt::format("Try to read {}",
                                              fname.generic_string()));
                }
                cb::io::tokenizeFileLineByLine(
                        fname, [&stats](const auto& parts) {
                            if (parts.size() < 1) {
                                return true;
                            }

                            // Total CPU time (in nanoseconds)
                            stats.usage = std::chrono::microseconds{
                                    stouint64(parts.front()) / 1000};
                            return true;
                        });
            }
        }

        if (cpu) {
            auto fname = *cpu / "cpu.stat";
            if (exists(fname)) {
                if (traceCallback) {
                    traceCallback(fmt::format("Try to read {}",
                                              fname.generic_string()));
                }
                cb::io::tokenizeFileLineByLine(
                        fname, [&stats](const auto& parts) {
                            if (parts.size() < 2) {
                                return true;
                            }
                            if (parts.front() == "nr_periods") {
                                stats.nr_periods = stouint64(parts[1]);
                            } else if (parts.front() == "nr_throttled") {
                                stats.nr_throttled = stouint64(parts[1]);
                            } else if (parts.front() == "throttled_time") {
                                stats.throttled = std::chrono::duration_cast<
                                        std::chrono::microseconds>(
                                        std::chrono::nanoseconds{
                                                stouint64(parts[1])});
                            } else if (parts.front() == "nr_bursts") {
                                stats.nr_bursts = stouint64(parts[1]);
                            } else if (parts.front() == "burst_time") {
                                stats.burst = std::chrono::duration_cast<
                                        std::chrono::microseconds>(
                                        std::chrono::nanoseconds{
                                                stouint64(parts[1])});
                            }
                            return true;
                        });
            }
        }

        return stats;
    }

    size_t get_max_memory() override {
        if (!memory) {
            return 0;
        }

        auto fname = *memory / "memory.limit_in_bytes";
        if (exists(fname)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", fname.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(fname, [&num](const auto& parts) {
                if (!parts.empty() && parts[0] != "-1") {
                    num = stouint64(parts[0]);
                }
                return true;
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

    size_t get_current_memory() override {
        if (!memory) {
            return 0;
        }

        auto fname = *memory / "memory.usage_in_bytes";
        if (exists(fname)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", fname.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(fname, [&num](const auto& parts) {
                if (!parts.empty()) {
                    num = stouint64(parts[0]);
                }
                return true;
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

    size_t get_current_cache_memory() override {
        if (!memory) {
            return 0;
        }

        auto fname = *memory / "memory.stat";
        if (exists(fname)) {
            size_t cache = 0;
            cb::io::tokenizeFileLineByLine(fname, [&cache](const auto& parts) {
                if (parts.size() > 1 && parts[0] == "cache") {
                    cache = stouint64(parts[1]);
                }
                return true;
            });
            return cache;
        }

        return 0;
    }

    std::optional<PressureData> get_pressure_data(PressureType type) override {
        return {};
    }

    bool hasController() {
        return cpu || cpuacct || memory;
    }

protected:
    size_t get_available_cpu_count_from_quota() override {
        if (!cpu) {
            return 0;
        }

        auto fname = *cpu / "cpu.cfs_period_us";
        size_t period = 100000;
        if (exists(fname)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", fname.generic_string()));
            }
            cb::io::tokenizeFileLineByLine(fname, [&period](const auto& parts) {
                if (!parts.empty()) {
                    period = stouint64(parts[0]);
                }
                return true;
            });
        }
        if (period == 0) {
            // Invalid period, but we don't want to end up with dividing
            // on 0
            return 0;
        }

        fname = *cpu / "cpu.cfs_quota_us";
        if (exists(fname)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", fname.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(
                    fname, [&num, period](const auto& parts) {
                        if (!parts.empty() && parts[0] != "-1") {
                            auto val = stouint64(parts[0]);
                            num = int((val * 100.0) / period);
                        }
                        return true;
                    });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

public:
    int user_hz;
    /// The location of the cpu controller if available
    std::optional<std::filesystem::path> cpu;
    /// The location of the cpu accounting controller if available
    std::optional<std::filesystem::path> cpuacct;
    /// The location of the memory controller if available
    std::optional<std::filesystem::path> memory;
};

class ControlGroupV2 : public ControlGroup {
public:
    ControlGroupV2(std::filesystem::path root, std::filesystem::path path)
        : ControlGroup(std::move(root)), directory(std::move(path)) {
        if (traceCallback) {
            traceCallback(fmt::format("Add V2 controller at {}",
                                      directory.generic_string()));
        }
    }

    Version get_version() override {
        return Version::V2;
    }

    CpuStat get_cpu_stats() override {
        CpuStat stats;

        auto file = directory / "cpu.stat";
        if (exists(file)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", file.generic_string()));
            }
            cb::io::tokenizeFileLineByLine(file, [&stats](const auto& parts) {
                if (parts.size() < 2) {
                    return true;
                }

                if (parts.front() == "usage_usec") {
                    stats.usage =
                            std::chrono::microseconds{stouint64(parts[1])};
                } else if (parts.front() == "user_usec") {
                    stats.user = std::chrono::microseconds{stouint64(parts[1])};
                } else if (parts.front() == "system_usec") {
                    stats.system =
                            std::chrono::microseconds{stouint64(parts[1])};
                } else if (parts.front() == "nr_periods") {
                    stats.nr_periods = stouint64(parts[1]);
                } else if (parts.front() == "nr_throttled") {
                    stats.nr_throttled = stouint64(parts[1]);
                } else if (parts.front() == "throttled_usec") {
                    stats.throttled =
                            std::chrono::microseconds{stouint64(parts[1])};
                } else if (parts.front() == "nr_bursts") {
                    stats.nr_bursts = stouint64(parts[1]);
                } else if (parts.front() == "burst_usec") {
                    stats.burst =
                            std::chrono::microseconds{stouint64(parts[1])};
                }
                return true;
            });
        }

        return stats;
    }

    size_t get_max_memory() override {
        auto file = directory / "memory.max";
        if (exists(file)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", file.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(file, [&num](const auto& parts) {
                if (!parts.empty() && parts[0] != "max") {
                    num = stouint64(parts[0]);
                }
                return true;
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

    size_t get_current_memory() override {
        auto file = directory / "memory.current";
        if (exists(file)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", file.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(file, [&num](const auto& parts) {
                if (!parts.empty()) {
                    num = stouint64(parts[0]);
                }
                return true;
            });
            if (num != 0) {
                return num;
            }
        }
        return 0;
    }

    size_t get_current_cache_memory() override {
        auto fname = directory / "memory.stat";
        if (exists(fname)) {
            size_t cache = 0;
            cb::io::tokenizeFileLineByLine(fname, [&cache](const auto& parts) {
                if (parts.size() > 1 && parts[0] == "file") {
                    cache = stouint64(parts[1]);
                }
                return true;
            });
            return cache;
        }

        return 0;
    }

    std::optional<PressureData> get_pressure_data(PressureType type) override {
        switch (type) {
        case PressureType::Cpu:
            return get_pressure_data_from_file(
                    directory / "cpu.pressure", PressureType::Cpu, false);
        case PressureType::Io:
            return get_pressure_data_from_file(
                    directory / "io.pressure", PressureType::Io, false);
        case PressureType::Memory:
            return get_pressure_data_from_file(
                    directory / "memory.pressure", PressureType::Memory, false);
        }

        throw std::invalid_argument("get_pressure_data(): Invalid type: " +
                                    std::to_string(int(type)));
    }

protected:
    size_t get_available_cpu_count_from_quota() override {
        auto file = directory / "cpu.max";
        if (exists(file)) {
            if (traceCallback) {
                traceCallback(
                        fmt::format("Try to read {}", file.generic_string()));
            }
            size_t num = 0;
            cb::io::tokenizeFileLineByLine(file, [&num](const auto& parts) {
                if (parts.size() > 1 && parts[0] != "max") {
                    auto v = stouint64(parts[0]);
                    auto p = stouint64(parts[1]);
                    if (p != 0) {
                        // we don't want to divide by zero (but getting 0
                        // would mean that the file is corrupt, but the
                        // analyzer tools don't know that)
                        num = int((v * 100.0) / p);
                    }
                }
                return true;
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

    const std::filesystem::path directory;
};

std::unique_ptr<ControlGroup> make_control_group(std::string root, pid_t pid) {
    auto mounts = priv::parse_proc_mounts(root);

    auto ret = std::make_unique<ControlGroupV1>(root.empty() ? "/" : root);
    if (traceCallback) {
        traceCallback("Try to configure V1 control groups"sv);
    }
    for (const auto& mp : mounts) {
        if (mp.type == priv::MountEntry::Version::V1) {
            auto path = find_cgroup_path(mp.path, pid);
            if (path) {
                ret->add_entry(*path, mp.option);
            }
        }
    }

    if (ret->hasController()) {
        // At least one of them was for a V1 (and we don't support a mix)
        return std::unique_ptr<ControlGroup>{ret.release()};
    }

    if (traceCallback) {
        traceCallback("No V1 control groups found. Try to configure V2"sv);
    }
    for (const auto& mp : mounts) {
        if (mp.type == priv::MountEntry::Version::V2) {
            auto path = find_cgroup_path(mp.path, pid);
            if (path) {
                return std::unique_ptr<ControlGroup>{
                        new ControlGroupV2(root.empty() ? "/" : root, *path)};
            }
        }
    }

    if (traceCallback) {
        traceCallback("No V1 or V2 control groups found"sv);
    }
    return std::unique_ptr<ControlGroup>{ret.release()};
}

} // namespace cb::cgroup::priv
