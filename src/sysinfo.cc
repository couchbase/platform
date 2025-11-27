/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/Unistd.h>

#ifdef __linux__
#include <cgroup/cgroup.h>
#endif

#ifdef HAVE_CPUID_H
#include <cpuid.h>
#endif

#include <platform/sysinfo.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(HAVE_SCHED_GETAFFINITY) || defined(HAVE_SCHED_GETCPU)
#include <sched.h>
#endif

#include <folly/concurrency/CacheLocality.h>

size_t cb::get_available_cpu_count() {
    char *override = getenv("COUCHBASE_CPU_COUNT");
    if (override != nullptr) {
        std::size_t pos;

        // std::stoi allows for leading whitespace, so we should allow for
        // trailing whitespace as well
        auto count = std::stoi(override, &pos);
        if (count > 0 && pos == strlen(override)) {
            return count;
        }

        // there might be characters after the number..
        const char* c = override + pos;
        do {
            if (!std::isspace(*c)) {
                throw std::logic_error(
                    "cb::get_available_cpu_count: Invalid format. COUCHBASE_CPU_COUNT should be a number");
            }
            c++;
        } while (*c);

        // the string had trailing spaces.. accept it anyway
        return count;
    }

#if defined(WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return size_t(sysinfo.dwNumberOfProcessors);
#elif defined(__linux__)
    return cb::cgroup::ControlGroup::instance().get_available_cpu_count();
#else
    // Fallback to sysconf
    auto ret = sysconf(_SC_NPROCESSORS_ONLN);
    if (ret == -1) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "cb::get_available_cpu_count(): sysconf failed");
    }

    return size_t(ret);
#endif
}

#if defined(WIN32)
static size_t groupSize = 0;
#endif

size_t cb::get_cpu_count() {
#if defined(WIN32)
    size_t logicalProcs = 0;
    const auto groups = GetMaximumProcessorGroupCount();
    for (WORD group = 0; group < groups; group++) {
        size_t currentGroupSize = GetMaximumProcessorCount(group);
        groupSize = std::max(groupSize, logicalProcs);
        logicalProcs += currentGroupSize;
    }
    return logicalProcs;
#else // !WIN32
    auto ret = sysconf(_SC_NPROCESSORS_CONF);
    if (ret == -1) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "cb::get_cpu_count(): sysconf failed");
    }

    return size_t(ret);
#endif // WIN32
}

size_t cb::stripe_for_current_cpu(size_t numStripes) {
    return folly::AccessSpreader<std::atomic>::cachedCurrent(numStripes);
}

size_t cb::get_num_last_level_cache() {
    return folly::CacheLocality::system().numCachesByLevel.back();
}
