/*
 *     Copyright 2019-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <platform/sysinfo.h>

#include <vector>

using CountFnType = size_t (*)();
using IndexFnType = size_t (*)(size_t);

/**
 * Store T to an element associated with the current "core" (cb::get_cpu_index)
 *
 * On construction, the number of cores is rounded up to the next power of two
 * and one T is allocated for each. This allows us to perform quick modulus
 * operations by using a bitmask which is important should the number of cores
 * increase at runtime.
 * The get() method then accesses to the callers current core T
 * The iterator (begin/end) allow a caller to access all elements e.g so all T
 * can be summed
 *
 * @tparam T type to be stored
 * @tparam CountFn A function ptr to a function to get the total cpu count. This
 *                 allows us to test environment specifics/changes by injecting
 *                 interesting values.
 * @tparam IndexFn A function ptr to a function to get the current cpu index.
 *                 This allows us to test environment specifics/changes by
 *                 injecting interesting values. Takes a size_t parameter that
 *                 should correspond to the number of available shards.
 */
template <typename T,
          CountFnType CountFn = cb::get_cpu_count,
          IndexFnType IndexFn = cb::stripe_for_current_cpu>
class CoreStore {
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using iterator = typename std::vector<T>::iterator;

    CoreStore() : coreArray(CountFn()) {
    }

    T& get() {
        return coreArray.at(IndexFn(coreArray.size()));
    }

    [[nodiscard]] size_t size() const {
        return coreArray.size();
    }

    [[nodiscard]] const_iterator begin() const {
        return coreArray.begin();
    }

    [[nodiscard]] const_iterator end() const {
        return coreArray.end();
    }

    iterator begin() {
        return coreArray.begin();
    }

    iterator end() {
        return coreArray.end();
    }

private:
    std::vector<T> coreArray;
};

/**
 * Version of CoreStore with the number of stripes set to the number of
 * last level caches in the system.
 *
 * See folly CacheLocality.h for details on access spreading.
 *
 * Useful intermediate step if a single instance demonstrates high
 * contention, but a default CoreStore with an instance per-core
 * could have unacceptable memory cost in large systems.
 */
template <class T>
using LastLevelCacheStore = CoreStore<T, cb::get_num_last_level_cache>;
