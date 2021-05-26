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
#else // !WIN32
#if defined(HAVE_SCHED_GETAFFINITY)
    // Prefer sched_getaffinity - number of CPUs we are permitted to
    // run on. This is useful when running in Docker or other similar
    // containers which report the 'full' host CPU count via
    // sysconf(_SC_NPROCESSORS_ONLN), but the configured --cpuset-cpus
    // via sched_getaffinity().
    cpu_set_t set;
    if (sched_getaffinity(getpid(), sizeof (set), &set) == 0) {
        return CPU_COUNT(&set);
    }
#endif
    // Fallback to sysconf
    auto ret = sysconf(_SC_NPROCESSORS_ONLN);
    if (ret == -1) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "cb::get_available_cpu_count(): sysconf failed");
    }

    return size_t(ret);
#endif // WIN32
}

#if defined(WIN32)
static size_t groupSize = 0;
#endif

size_t cb::get_cpu_count() {
#if defined(WIN32)
    size_t logicalProcs = 0;
    int groups = GetMaximumProcessorGroupCount();
    for (int group = 0; group < groups; group++) {
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

size_t cb::get_cpu_index() {
#if defined(WIN32)
    if (groupSize == 0) {
        cb::get_cpu_count();
    }
    PROCESSOR_NUMBER processor;
    GetCurrentProcessorNumberEx(&processor);
    return processor.Number + (processor.Group * groupSize);
#elif defined(HAVE_SCHED_GETCPU)
    int rv = sched_getcpu();
    if (rv == -1) {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "cb::get_cpu_index(): sched_getcpu failed");
    }
    return rv;
#elif defined(__APPLE__)
    // From:
    // https://github.com/apple/darwin-xnu/blob/0a798f6738bc1db01281fc08ae024145e84df927/libsyscall/os/tsd.h
    // macOS stores the cpu number in the lower bits of the IDTR.
    {
#if defined(__x86_64__) || defined(__i386__)
        struct __attribute__((packed)) {
            uint16_t size;
            uintptr_t ptr;
        } idt;
        __asm__("sidt %[p]" : [p] "=&m"(idt));
        return size_t(idt.size & 0xfff);
#else
#error get_cpu_index (macOS) not implemented on this architecture
#endif
    }
#else // !HAVE_SCHED_GETCPU
    uint32_t registers[4] = {0, 0, 0, 0};
    __cpuid(1, registers[0], registers[1], registers[2], registers[3]);
    return registers[1] >> 24;
#endif
}

size_t cb::stripe_for_current_cpu(size_t numStripes) {
    return folly::AccessSpreader<std::atomic>::cachedCurrent(numStripes);
}

size_t cb::get_num_last_level_cache() {
    return folly::CacheLocality::system().numCachesByLevel.back();
}
