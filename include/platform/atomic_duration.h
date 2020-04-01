/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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

#include <atomic>
#include <chrono>

namespace cb {
/* Wrapper class around std::atomic. It provides atomic operations
 * for std::chrono durations by operating on the underlying value
 * obtained from the duration's count().
 * Defaults to relaxed memory ordering, suitable for statistics.
 */
template <std::memory_order MemoryOrder =
                  std::memory_order::memory_order_relaxed>
class AtomicDuration {
public:
    AtomicDuration() {
        store(std::chrono::steady_clock::duration::zero());
    }

    AtomicDuration(std::chrono::steady_clock::duration initial) {
        store(initial);
    }

    explicit AtomicDuration(const AtomicDuration& other) {
        store(other.load());
    }

    operator std::chrono::steady_clock::duration() const {
        return load();
    }

    [[nodiscard]] std::chrono::steady_clock::duration load() const {
        return std::chrono::steady_clock::duration(value.load(MemoryOrder));
    }

    void store(std::chrono::steady_clock::duration desired) {
        value.store(desired.count(), MemoryOrder);
    }

    std::chrono::steady_clock::duration fetch_add(
            std::chrono::steady_clock::duration arg) {
        return std::chrono::steady_clock::duration(
                value.fetch_add(arg.count(), MemoryOrder));
    }

    std::chrono::steady_clock::duration fetch_sub(
            std::chrono::steady_clock::duration arg) {
        return std::chrono::steady_clock::duration(
                value.fetch_sub(arg.count(), MemoryOrder));
    }

    AtomicDuration& operator=(std::chrono::steady_clock::duration val) {
        store(val);
        return *this;
    }

    AtomicDuration& operator+=(std::chrono::steady_clock::duration rhs) {
        fetch_add(rhs);
        return *this;
    }

    AtomicDuration& operator-=(std::chrono::steady_clock::duration rhs) {
        fetch_sub(rhs);
        return *this;
    }

    std::chrono::steady_clock::duration operator++() {
        return fetch_add(std::chrono::steady_clock::duration(1)) +
               std::chrono::steady_clock::duration(1);
    }

    std::chrono::steady_clock::duration operator++(int) {
        return fetch_add(std::chrono::steady_clock::duration(1));
    }

    std::chrono::steady_clock::duration operator--() {
        return fetch_sub(std::chrono::steady_clock::duration(1)) -
               std::chrono::steady_clock::duration(1);
    }

    std::chrono::steady_clock::duration operator--(int) {
        return fetch_sub(std::chrono::steady_clock::duration(1));
    }

private:
    std::atomic<std::chrono::steady_clock::duration::rep> value;
};
} // namespace cb
