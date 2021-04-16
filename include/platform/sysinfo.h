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
size_t get_available_cpu_count();

/**
 * Returns the number of logical threads (CPUs) this process has
 * access to - i.e. the maximum number of concurrent threads of
 * execution available.
 *
 * @throws std::runtime_error if the system call to fetch available CPUs fail
 */
size_t get_cpu_count();

/**
 * @return the current CPU of the caller
 * @throws std::runtime_error if the system call to fetch current CPU fails
 */
size_t get_cpu_index();

/**
 * Get a (potentially cached) stripe index for the current core.
 * One or more cores may be mapped to a given stripe; if numStripes
 * is equal to the number of cores they will be mapped 1-to-1, if there
 * are fewer stripes than cores, multiple cores will share a stripe.
 * See folly::AccessSpreader for core to stripe allocation logic.
 *
 * @param numStripes number of stripes
 * @return stripe index for the current cpu core.
 */
size_t stripe_for_current_cpu(size_t numStripes);

/**
 * Get the number of last level caches in the system.
 */
size_t get_num_last_level_cache();
}

// For backwards compatibility
namespace Couchbase {
inline size_t get_available_cpu_count() {
    return cb::get_available_cpu_count();
}
}
