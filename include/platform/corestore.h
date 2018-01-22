/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc
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

#include <stdexcept>
#include <string>
#include <vector>

/**
 * Store T to an element associated with the current "core" (cb::get_cpu_index)
 *
 * One T is allocated per core (number of cores determined by cb::get_cpu_count)
 * The get() method then accesses to the callers current core T
 * The iterator (begin/end) allow a caller to access all elements e.g so all T
 * can be summed
 *
 * @tparam T type to be stored
 */
template <typename T>
class CoreStore {
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using iterator = typename std::vector<T>::iterator;

    CoreStore() : coreArray(cb::get_cpu_count()) {
    }

    T& get() {
        auto index = cb::get_cpu_index();
        if (index > coreArray.size()) {
            throw std::out_of_range("CoreStore::get index:" +
                                    std::to_string(index) + " out of bounds:" +
                                    std::to_string(coreArray.size()));
        }
        return coreArray.at(index);
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
