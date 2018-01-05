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

#include "config.h"

#include <atomic>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace cb {

/// Policy class for handling underflow by clamping the value at zero.
template <class T>
struct ClampAtZeroUnderflowPolicy {
    void underflow(T& newValue) {
        newValue = 0;
    }
};

/// Policy class for handling underflow by throwing an exception.
template <class T>
struct ThrowExceptionUnderflowPolicy {
    void underflow(T& newValue) {
        using std::to_string;
        throw std::underflow_error("ThrowExceptionUnderflowPolicy newValue:" +
                                   to_string(newValue));
    }
};

// Default NonNegativeCounter OrdereReversedPolicy (if user doesn't explicitly
// specify otherwise) - use ClampAtZeroUnderflowPolicy for Release builds, and
// ThrowExceptionPolicy for Pre-Release builds.
template <class T>
#if CB_DEVELOPMENT_ASSERTS
using DefaultUnderflowPolicy = ThrowExceptionUnderflowPolicy<T>;
#else
using DefaultUnderflowPolicy = ClampAtZeroUnderflowPolicy<T>;
#endif

/**
 * The NonNegativeCounter class wraps std::atomic<> and prevents it
 * underflowing. By default will clamp the value at 0 on underflow, but
 * behaviour can be customized by specifying a different UnderflowPolicy class.
 */
template <typename T,
          template <class> class UnderflowPolicy = DefaultUnderflowPolicy>
class NonNegativeCounter : public UnderflowPolicy<T> {
    static_assert(
            std::is_unsigned<T>::value,
            "NonNegativeCounter should only be templated over unsigned types");

public:
    NonNegativeCounter() {
        store(0);
    }

    NonNegativeCounter(const T& initial) {
        store(initial);
    }

    NonNegativeCounter(const NonNegativeCounter& other) {
        store(other.load());
    }

    operator T() const {
        return load();
    }

    T load() const {
        return value.load(std::memory_order_relaxed);
    }

    void store(T desired) {
        value.store(desired, std::memory_order_relaxed);
    }

    T fetch_add(T arg) {
        return value.fetch_add(arg, std::memory_order_relaxed);
    }

    T fetch_sub(T arg) {
        T expected = load();
        T desired;
        do {
            if (expected < arg) {
                UnderflowPolicy<T>::underflow(desired);
            } else {
                desired = expected - arg;
            }
        } while (!value.compare_exchange_weak(
                expected, desired, std::memory_order_relaxed));

        return expected;
    }

    T exchange(T arg) {
        return value.exchange(arg, std::memory_order_relaxed);
    }

    NonNegativeCounter& operator=(const NonNegativeCounter& rhs) {
        store(rhs.load());
        return *this;
    }

    NonNegativeCounter& operator+=(const T rhs) {
        fetch_add(rhs);
        return *this;
    }

    NonNegativeCounter& operator+=(const NonNegativeCounter& rhs) {
        fetch_add(rhs.load());
        return *this;
    }

    NonNegativeCounter& operator-=(const T rhs) {
        fetch_sub(rhs);
        return *this;
    }

    NonNegativeCounter& operator-=(const NonNegativeCounter& rhs) {
        fetch_sub(rhs.load());
        return *this;
    }

    T operator++() {
        return fetch_add(1) + 1;
    }

    T operator++(int) {
        return fetch_add(1);
    }

    T operator--() {
        T previous = fetch_sub(1);
        if (previous == 0) {
            UnderflowPolicy<T>::underflow(previous);
            return 0;
        }
        return previous - 1;
    }

    T operator--(int) {
        return fetch_sub(1);
    }

    NonNegativeCounter& operator=(T val) {
        store(val);
        return *this;
    }

private:
    std::atomic<T> value;
};

} // namespace cb
