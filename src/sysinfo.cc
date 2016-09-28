/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "config.h"

#include <platform/sysinfo.h>

#if defined(HAVE_SCHED_GETAFFINITY)
#include <sched.h>
#endif
#include <unistd.h>

size_t Couchbase::get_available_cpu_count() {
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
    return size_t(sysconf(_SC_NPROCESSORS_ONLN));
#endif // WIN32
}
