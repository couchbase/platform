/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2019 Couchbase, Inc
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

#include <platform/sysinfo.h>

#include <folly/concurrency/CacheLocality.h>
#include <cmath>
#include <stdexcept>
#include <string>
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
          IndexFnType IndexFn =
                  folly::AccessSpreader<std::atomic>::cachedCurrent>
class CoreStore {
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using iterator = typename std::vector<T>::iterator;

    CoreStore() : coreArray(CountFn()) {
    }

    T& get() {
        return coreArray.at(IndexFn(coreArray.size()));
    }

    size_t size() const {
        return coreArray.size();
    }

    const_iterator begin() const {
        return coreArray.begin();
    }

    const_iterator end() const {
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
