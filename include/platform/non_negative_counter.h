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
    void underflow(T& desired, T current, T arg) {
        desired = 0;
    }
};

/// Policy class for handling underflow by throwing an exception. Prints the
/// previous value stored in the counter and the argument (the value that we
// were attempting to subtract)
template <class T>
struct ThrowExceptionUnderflowPolicy {
    void underflow(T& desired, T current, T arg) {
        using std::to_string;
        throw std::underflow_error("ThrowExceptionUnderflowPolicy current:" +
                                   to_string(current) + " arg:" +
                                   to_string(arg));
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
        T current = load();
        T desired;
        do {
            if (current < arg) {
                UnderflowPolicy<T>::underflow(desired, current, arg);
            } else {
                desired = current - arg;
            }
            // Attempt to set the atomic value to desired. If the atomic value
            // is not the same as current then it has changed during
            // operation. compare_exchange_weak will reload the new value
            // into current if it fails, and we will retry.
        } while (!value.compare_exchange_weak(
                current, desired, std::memory_order_relaxed));

        return current;
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
            // If we are doing a clamp underflow we can pass in previous,
            // it's already 0 and we are returning 0. If we are going to
            // throw, we want to print previous.
            UnderflowPolicy<T>::underflow(previous, previous, 1);
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
