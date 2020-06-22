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
 * @tparam Duration The duration type to use, defaults to steady_clock
 *         duration.
 * @tparam MemoryOrder The memory ordering to use for atomic operations,
 *         defaults to relaxed memory ordering, suitable for statistics.
 */
template <typename Duration = std::chrono::steady_clock::duration,
          std::memory_order MemoryOrder =
                  std::memory_order::memory_order_relaxed>
class AtomicDuration {
public:
    AtomicDuration() {
        store(Duration::zero());
    }

    AtomicDuration(Duration initial) {
        store(initial);
    }

    explicit AtomicDuration(const AtomicDuration& other) {
        store(other.load());
    }

    operator Duration() const {
        return load();
    }

    [[nodiscard]] Duration load() const {
        return Duration(value.load(MemoryOrder));
    }

    void store(Duration desired) {
        value.store(desired.count(), MemoryOrder);
    }

    Duration fetch_add(Duration arg) {
        return Duration(value.fetch_add(arg.count(), MemoryOrder));
    }

    Duration fetch_sub(Duration arg) {
        return Duration(value.fetch_sub(arg.count(), MemoryOrder));
    }

    AtomicDuration& operator=(Duration val) {
        store(val);
        return *this;
    }

    AtomicDuration& operator+=(Duration rhs) {
        fetch_add(rhs);
        return *this;
    }

    AtomicDuration& operator-=(Duration rhs) {
        fetch_sub(rhs);
        return *this;
    }

    Duration operator++() {
        return fetch_add(Duration(1)) + Duration(1);
    }

    Duration operator++(int) {
        return fetch_add(Duration(1));
    }

    Duration operator--() {
        return fetch_sub(Duration(1)) - Duration(1);
    }

    Duration operator--(int) {
        return fetch_sub(Duration(1));
    }

private:
    std::atomic<typename Duration::rep> value;

    static_assert(
            decltype(value)::is_always_lock_free,
            "This instantiation of AtomicDuration<> is not lock-free. If you "
            "don't need a lock-free "
            "atomic duration, std::atomic<Duration> might as well be used.");
};

/* Wrapper class around std::atomic. It provides atomic operations
 * for std::chrono time_points by operating on the underlying value
 * obtained from the time_point's representation..
 * @tparam Clock The time_point type to use, defaults to steady_clock time_point.
 * @tparam MemoryOrder The memory ordering to use for atomic operations,
 *         defaults to relaxed memory ordering, suitable for statistics.
 */
template <typename TimePoint = std::chrono::steady_clock::time_point,
        std::memory_order MemoryOrder =
        std::memory_order::memory_order_relaxed>
class AtomicTimePoint {
public:
    AtomicTimePoint() {
        store(TimePoint{});
    }

    AtomicTimePoint(TimePoint initial) {
        store(initial);
    }

    AtomicTimePoint& operator=(TimePoint val) {
        store(val);
        return *this;
    }

    operator TimePoint() const {
        return load();
    }

    [[nodiscard]] TimePoint load() const {
        return TimePoint{typename TimePoint::duration{value.load(MemoryOrder)}};
    }

    void store(TimePoint desired) {
        value.store(desired.time_since_epoch().count(), MemoryOrder);
    }

private:
    std::atomic<typename TimePoint::rep> value;

    static_assert(
            decltype(value)::is_always_lock_free,
            "This instantiation of AtomicTimePoint<> is not lock-free. If you "
            "don't need a lock-free atomic duration, std::atomic<TimePoint> "
            "might as well be used.");
};

} // namespace cb
