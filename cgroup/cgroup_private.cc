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
#include <boost/filesystem.hpp>
#include <cgroup/cgroup.h>
#include <unistd.h>
#include <cerrno>
#include <deque>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace cb::cgroup::priv {

static std::vector<std::string> split_string(std::string_view s, char delim) {
    std::vector<std::string> result;

    size_t n = 0;
    while (true) {
        const auto m = s.find(delim, n);
        if (m == std::string::npos) {
            break;
        }

        result.emplace_back(s, n, m - n);
        n = m + 1;
    }
    result.emplace_back(s, n, s.size() - n);
    auto& back = result.back();
    while (!back.empty() && back.back() == '\n') {
        back.pop_back();
    }
    if (back.empty()) {
        result.pop_back();
    }

    return result;
}

/// Read the provided file and call the callback for every line (but tokenize
/// the line by using the provided delimeter.
/// throws std::system_error if an error occurs
static void read_file(boost::filesystem::path file,
                      std::function<void(std::vector<std::string>)> callback,
                      char delim = ' ') {
    std::vector<char> data(1024);
    struct FileDeleter {
        void operator()(FILE* fp) {
            fclose(fp);
        }
    };

    std::unique_ptr<FILE, FileDeleter> fp(
            fopen(file.generic_string().c_str(), "r"));
    if (!fp) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "cb::cgroup::read_file(): Failed to open " +
                                        file.generic_string());
    }

    while (fgets(data.data(), data.size(), fp.get())) {
        auto parts = split_string(data.data(), delim);
        callback(parts);
    }
}

struct MountEntry {
    enum class Version { V1, V2 };
    MountEntry(std::string_view t, const std::string& p, std::string o)
        : type(t == "cgroup2" ? Version::V2 : Version::V1),
          path(p),
          option(std::move(o)) {
    }
    const Version type;
    const boost::filesystem::path path;
    const std::string option;
};

/// Read out the cgroups entries from /proc/mounts
/// throws std::system_error on errors
static std::vector<MountEntry> parse_proc_mounts(const std::string root) {
    std::vector<MountEntry> ret;
    read_file(root + "/proc/mounts", [&ret](std::vector<std::string> parts) {
        // pick out the lines which looks like:
        //   cgroup[2] /sys/fs/cgroup cgroup[2] rw,nosuid,optionblah
        if (parts.size() > 3 &&
            parts.front().find("cgroup") != std::string::npos) {
            ret.emplace_back(
                    MountEntry{parts[2], parts[1], std::move(parts[3])});
        }
    });
    return ret;
}

static bool search_file(pid_t pid, const boost::filesystem::path& file) {
    bool found = false;
    const auto textual = std::to_string(pid);
    read_file(file, [&found, &textual](std::vector<std::string> parts) {
        if (!parts.empty() && parts.front() == textual) {
            found = true;
        }
    });
    return found;
}

/**
 * Search the subtree from root for all the cgroup.procs files where the
 * current process is located.
 *
 * @param root the root directory to search in
 * @return the path to where I found the pid registered
 * @thros std::system_error for io errors
 */
static std::optional<boost::filesystem::path> find_cgroup_path(
        const boost::filesystem::path& root) {
    const auto pid = getpid();
    std::vector<boost::filesystem::path> ret;
    std::deque<boost::filesystem::path> paths;
    paths.push_back(root);
    while (!paths.empty()) {
        auto path = paths.front();
        paths.pop_front();

        auto file = path / "cgroup.procs";
        if (exists(file) && search_file(pid, file)) {
            return path;
        }

        for (const auto& p : boost::filesystem::directory_iterator(path)) {
            if (is_directory(p) && !is_symlink(p)) {
                paths.push_back(p.path());
            }
        }
    }

    return {};
}

class ControlGroupV1 : public ControlGroup {
public:
    ControlGroupV1() {
        user_hz = sysconf(_SC_CLK_TCK);
        if (user_hz == -1) {
            user_hz = 100;
        }
    }

    void add_entry(const boost::filesystem::path& path,
                   std::string_view options) {
        auto tokens = split_string(options, ',');
        for (const auto& token : tokens) {
            if (token == "cpu") {
                cpu = path;
            }
            if (token == "cpuacct") {
                cpuacct = path;
            }
            if (token == "memory") {
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
                read_file(fname,
                          [&stats, this](std::vector<std::string> parts) {
                              if (parts.size() < 2) {
                                  return;
                              }

                              // CPU time is reported in the units defined by
                              // the USER_HZ variable (for some weird reason).
                              // This value is typically set to 100
                              if (parts.front() == "user") {
                                  stats.user = std::chrono::microseconds{
                                          std::stoull(parts[1]) *
                                          (1000 / user_hz) * 1000};
                              } else if (parts.front() == "system") {
                                  stats.system = std::chrono::microseconds{
                                          std::stoull(parts[1]) *
                                          (1000 / user_hz) * 1000};
                              }
                          });
            }
            fname = *cpuacct / "cpuacct.usage";
            if (exists(fname)) {
                read_file(fname, [&stats](std::vector<std::string> parts) {
                    if (parts.size() < 1) {
                        return;
                    }

                    // Total CPU time (in nanoseconds)
                    stats.usage = std::chrono::microseconds{
                            std::stoull(parts.front()) / 1000};
                });
            }
        }

        if (cpu) {
            auto fname = *cpu / "cpu.stat";
            if (exists(fname)) {
                read_file(fname, [&stats](std::vector<std::string> parts) {
                    if (parts.size() < 2) {
                        return;
                    }
                    if (parts.front() == "nr_periods") {
                        stats.nr_periods = std::stoull(parts[1]);
                    } else if (parts.front() == "nr_throttled") {
                        stats.nr_throttled = std::stoull(parts[1]);
                    } else if (parts.front() == "throttled_time") {
                        stats.throttled = std::chrono::microseconds{
                                std::stoull(parts[1])};
                    }
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
            size_t num = 0;
            read_file(fname, [&num](std::vector<std::string> parts) {
                if (!parts.empty()) {
                    num = std::stoull(parts[0]);
                    if (num == std::stoull("-1")) {
                        num = 0;
                    }
                }
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
            size_t num = 0;
            read_file(fname, [&num](std::vector<std::string> parts) {
                if (!parts.empty()) {
                    num = std::stoull(parts[0]);
                }
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

protected:
    size_t get_available_cpu_count_from_quota() override {
        if (!cpu) {
            return 0;
        }

        auto fname = *cpu / "cpu.cfs_period_us";
        size_t period = 100000;
        if (exists(fname)) {
            read_file(fname, [&period](std::vector<std::string> parts) {
                if (!parts.empty()) {
                    period = std::stoi(parts[0]);
                }
            });
        }

        fname = *cpu / "cpu.cfs_quota_us";
        if (exists(fname)) {
            size_t num = 0;
            read_file(fname, [&num, period](std::vector<std::string> parts) {
                if (!parts.empty()) {
                    auto val = std::stoi(parts[0]);
                    if (val != -1) {
                        num = val / period;
                        if (val % period) {
                            num++;
                        }
                    }
                }
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
    std::optional<boost::filesystem::path> cpu;
    /// The location of the cpu accounting controller if available
    std::optional<boost::filesystem::path> cpuacct;
    /// The location of the memory controller if available
    std::optional<boost::filesystem::path> memory;
};

class ControlGroupV2 : public ControlGroup {
public:
    ControlGroupV2(boost::filesystem::path path) : directory(std::move(path)) {
    }

    Version get_version() override {
        return Version::V2;
    }

    CpuStat get_cpu_stats() override {
        CpuStat stats;

        auto file = directory / "cpu.stat";
        if (exists(file)) {
            read_file(file, [&stats](std::vector<std::string> parts) {
                if (parts.size() < 2) {
                    return;
                }

                if (parts.front() == "usage_usec") {
                    stats.usage =
                            std::chrono::microseconds{std::stoull(parts[1])};
                } else if (parts.front() == "user_usec") {
                    stats.user =
                            std::chrono::microseconds{std::stoull(parts[1])};
                } else if (parts.front() == "system_usec") {
                    stats.system =
                            std::chrono::microseconds{std::stoull(parts[1])};
                } else if (parts.front() == "nr_periods") {
                    stats.nr_periods = std::stoull(parts[1]);
                } else if (parts.front() == "nr_throttled") {
                    stats.nr_throttled = std::stoull(parts[1]);
                } else if (parts.front() == "throttled_usec") {
                    stats.throttled =
                            std::chrono::microseconds{std::stoull(parts[1])};
                } else if (parts.front() == "nr_bursts") {
                    stats.nr_bursts = std::stoull(parts[1]);
                } else if (parts.front() == "burst_usec") {
                    stats.burst =
                            std::chrono::microseconds{std::stoull(parts[1])};
                }
            });
        }

        return stats;
    }

    size_t get_max_memory() override {
        auto file = directory / "memory.max";
        if (exists(file)) {
            size_t num = 0;
            read_file(file, [&num](std::vector<std::string> parts) {
                if (!parts.empty() && parts[0] != "max") {
                    num = std::stoull(parts[0]);
                }
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
            size_t num = 0;
            read_file(file, [&num](std::vector<std::string> parts) {
                if (!parts.empty()) {
                    num = std::stoull(parts[0]);
                }
            });
            if (num != 0) {
                return num;
            }
        }
        return 0;
    }

protected:
    size_t get_available_cpu_count_from_quota() override {
        auto file = directory / "cpu.max";
        if (exists(file)) {
            size_t num = 0;
            read_file(file, [&num](std::vector<std::string> parts) {
                if (parts.size() > 1 && parts[0] != "max") {
                    auto v = std::stoi(parts[0]);
                    auto p = std::stoi(parts[1]);
                    num = v / p;
                    if (v % p) {
                        ++num;
                    }
                }
            });
            if (num != 0) {
                return num;
            }
        }

        return 0;
    }

    const boost::filesystem::path directory;
};

std::unique_ptr<ControlGroup> make_control_group(std::string root) {
    auto mounts = priv::parse_proc_mounts(root);

    // prefer V2
    for (const auto& mp : mounts) {
        if (mp.type == priv::MountEntry::Version::V2) {
            auto path = find_cgroup_path(mp.path);
            if (path) {
                return std::unique_ptr<ControlGroup>{new ControlGroupV2(*path)};
            }
        }
    }

    auto ret = std::make_unique<ControlGroupV1>();
    for (const auto& mp : mounts) {
        if (mp.type == priv::MountEntry::Version::V1) {
            auto path = find_cgroup_path(mp.path);
            if (path) {
                ret->add_entry(*path, mp.option);
            }
        }
    }

    return std::unique_ptr<ControlGroup>{ret.release()};
}

} // namespace cb::cgroup::priv
