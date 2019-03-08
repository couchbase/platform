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
#pragma once

#include <platform/visibility.h>
#include <cstddef>

namespace cb {

/**
 * Returns the number of logical threads (CPUs) this process has
 * access to - i.e. the maximum number of concurrent threads of
 * execution available.
 *
 * The user may override the number of CPUs to use by using the
 * environment variable COUCHBASE_CPU_COUNT
 *
 * @throws std::logic_error if the environemnt variable can't be parsed
 *         std::runtime_error if the system call to fetch available CPUs fail
 */
PLATFORM_PUBLIC_API
size_t get_available_cpu_count();

/**
 * Returns the number of logical threads (CPUs) this process has
 * access to - i.e. the maximum number of concurrent threads of
 * execution available.
 *
 * @throws std::runtime_error if the system call to fetch available CPUs fail
 */
PLATFORM_PUBLIC_API
size_t get_cpu_count();

/**
 * @return the current CPU of the caller
 * @throws std::runtime_error if the system call to fetch current CPU fails
 */
PLATFORM_PUBLIC_API
size_t get_cpu_index();
}

// For backwards compatibility
namespace Couchbase {
inline size_t get_available_cpu_count() {
    return cb::get_available_cpu_count();
}
}
