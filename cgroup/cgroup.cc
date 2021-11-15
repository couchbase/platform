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
    int num = get_available_cpu_count_from_environment();
    if (num != 0) {
        return num;
    }

    num = get_available_cpu_count_from_quota();
    if (num != 0) {
        return num;
    }

    // No quota set, check for cpu sets
    cpu_set_t set;
    if (sched_getaffinity(getpid(), sizeof(set), &set) == 0) {
        return CPU_COUNT(&set);
    }

    auto ret = sysconf(_SC_NPROCESSORS_ONLN);
    if (ret == -1) {
        throw std::system_error(
                std::error_code(errno, std::system_category()),
                "cb::cgroup::get_available_cpu_count(): sysconf failed");
    }

    return size_t(ret);
}

size_t ControlGroup::get_available_cpu_count_from_environment() {
    char* env = getenv("COUCHBASE_CPU_COUNT");
    if (env == nullptr) {
        return 0;
    }
    std::size_t pos;

    // std::stoi allows for leading whitespace, so we should allow for
    // trailing whitespace as well
    auto count = std::stoi(env, &pos);
    if (count > 0 && pos == strlen(env)) {
        return count;
    }

    // there might be characters after the number...
    const char* c = env + pos;
    do {
        if (!std::isspace(*c)) {
            throw std::logic_error(
                    "cb::cgroup::get_available_cpu_count: Invalid format. "
                    "COUCHBASE_CPU_COUNT should be a number");
        }
        c++;
    } while (*c);

    // the string had trailing spaces.. accept it anyway
    return count;
}

ControlGroup& ControlGroup::instance() {
    static auto instance = priv::make_control_group();
    return *instance;
}
} // namespace cb::cgroup
