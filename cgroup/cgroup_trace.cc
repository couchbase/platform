/*
 *    Copyright 2022-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cgroup/cgroup.h>
#include <nlohmann/json.hpp>
#include <iostream>

int main() {
    cb::cgroup::ControlGroup::setTraceCallback(
            [](auto msg) { std::cout << msg << std::endl; });
    try {
        auto& instance = cb::cgroup::ControlGroup::instance();
        const auto stats = instance.get_cpu_stats();
        nlohmann::json json{{"num_cpu_prc", instance.get_available_cpu()},
                            {"memory_max", instance.get_max_memory()},
                            {"memory_current", instance.get_current_memory()},
                            {"usage_usec", stats.usage.count()},
                            {"user_usec", stats.user.count()},
                            {"system_usec", stats.system.count()},
                            {"nr_periods", stats.nr_periods},
                            {"nr_throttled", stats.nr_throttled},
                            {"throttled_usec", stats.throttled.count()},
                            {"nr_bursts", stats.nr_bursts},
                            {"burst_usec", stats.burst.count()}};

        std::cout << std::endl
                  << std::endl
                  << "Got the following information: " << std::endl
                  << json.dump(2) << std::endl;
    } catch (const std::exception& exception) {
        std::cerr << "ERROR: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
