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
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <system_error>

namespace cb::cgroup {

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

    // No quota set, check for cpu sets
    cpu_set_t set;
    if (sched_getaffinity(getpid(), sizeof(set), &set) == 0) {
        return CPU_COUNT(&set) * 100;
    }

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
} // namespace cb::cgroup
