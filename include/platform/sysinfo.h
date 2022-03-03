/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
