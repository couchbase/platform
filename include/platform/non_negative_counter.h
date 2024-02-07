/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <atomic>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <platform/exceptions.h>

namespace cb {

/// Policy class for handling underflow by clamping the value at zero.
template <class T>
struct ClampAtZeroUnderflowPolicy {
    using SignedT = typename std::make_signed<T>::type;

    void underflow(T& desired, T current, SignedT arg) {
        desired = 0;
    }
};

/// Policy class for handling underflow by throwing an exception. Prints the
/// previous value stored in the counter and the argument (the value that we
// were attempting to subtract)
template <class T>
struct ThrowExceptionUnderflowPolicy {
    using SignedT = typename std::make_signed<T>::type;

    void underflow(T& desired, T current, SignedT arg) {
        using std::to_string;
        cb::throwWithTrace(std::underflow_error("ThrowExceptionUnderflowPolicy current:" +
                                   to_string(current) + " arg:" +
                                   to_string(arg)));
    }
};

/// Policy class for handling underflow dynamically based on the current mode
/// of the object. Allows runtime selection of how underflows are handled.
template <class T>
struct DynamicUnderflowPolicy {
    using SignedT = typename std::make_signed<T>::type;

    enum class Mode { ClampAtZero, ThrowException };

    Mode mode{Mode::ClampAtZero};

    void underflow(T& desired, T current, SignedT arg) {
        switch (mode) {
        case Mode::ClampAtZero:
            clampAtZero.underflow(desired, current, arg);
            return;
        case Mode::ThrowException:
            throwException.underflow(desired, current, arg);
            return;
        }
    }
    ClampAtZeroUnderflowPolicy<T> clampAtZero;
    ThrowExceptionUnderflowPolicy<T> throwException;
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
 * The NonNegativeCounter class wraps T and prevents it underflowing
 * or overflowing. By default will clamp the value at 0 on underflow,
 * but behaviour can be customized by specifying a different
 * UnderflowPolicy class. Same for overflow.
 *
 * Even though this counter can only be templated on unsigned types, it has the
 * maximum value of the corresponding signed type. This is because we need to
 * allow and check the addition of negative values.
 */
template <typename T,
          template <class> class UnderflowPolicy = DefaultUnderflowPolicy>
class NonNegativeCounter : public UnderflowPolicy<T> {
    static_assert(
            std::is_unsigned<T>::value,
            "NonNegativeCounter should only be templated over unsigned types");

    using SignedT = typename std::make_signed<T>::type;

public:
    using value_type = T;

    NonNegativeCounter() = default;

    NonNegativeCounter(T initial) {
        store(initial);
    }

    operator T() const noexcept {
        return load();
    }

    [[nodiscard]] T load() const noexcept {
        return value;
    }

    void store(T desired) {
        if (desired > T(std::numeric_limits<SignedT>::max())) {
            UnderflowPolicy<T>::underflow(desired, load(), desired);
        }
        value = desired;
    }

    /**
     * Add 'arg' to the current value. If the new value would underflow (i.e. if
     * arg was negative and current less than arg) then calls underflow() on the
     * selected UnderflowPolicy.
     *
     * Note: Not marked 'noexcept' as underflow() could throw.
     */
    T fetch_add(SignedT arg) {
        T current = load();
        T desired;
        if (arg < 0) {
            desired = current - T(std::abs(arg));
            if (SignedT(current) + arg < 0) {
                UnderflowPolicy<T>::underflow(desired, current, arg);
            }
        } else {
            desired = current + T(arg);
            if (desired > T(std::numeric_limits<SignedT>::max())) {
                UnderflowPolicy<T>::underflow(desired, current, arg);
            }
        }
        value = desired;
        return current;
    }

    /**
     * Subtract 'arg' from the current value. If the new value would underflow
     * then calls underflow() on the selected UnderflowPolicy.
     *
     * Note: Not marked 'noexcept' as underflow() could throw.
     */
    T fetch_sub(SignedT arg) {
        T current = load();
        T desired;
        if (arg < 0) {
            desired = current + T(std::abs(arg));
        } else {
            desired = current - T(arg);
        }

        if (desired > T(std::numeric_limits<SignedT>::max())) {
            UnderflowPolicy<T>::underflow(desired, current, arg);
        }
	value = desired;
        return current;
    }

    T exchange(T arg) noexcept {
        std::swap(value, arg);
        return arg;
    }

    NonNegativeCounter& operator=(T val) {
        store(val);
        return *this;
    }

    NonNegativeCounter& operator+=(const T rhs) {
        fetch_add(rhs);
        return *this;
    }

    NonNegativeCounter& operator-=(const T rhs) {
        fetch_sub(rhs);
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

private:
    T value{0};
};

/**
 * The AtomicNonNegativeCounter is an atomic version of NonNegativeCounter -
 * the template type T is wrapped in std::atomic<>, and all updates / checks
 * are performed atomically (e.g. using compare-exchange where required) so
 * this class is safe to use concurrently from multiple threads.
 *
 *     Implementation Note: while it might seem like we can implement this in
 *     terms of NonNegativeCounter (say by templating that on std::atomic<T>
 *     instead of T), this is tricker than it may initially seem; as we still
 *     want various methods to take / return the underlying 'T' type and
 *     not an atomic version of it.
 *     As such, this is implemented as a copy of NonNegativeCounter but
 *     with tweaks to use atomic functions where necessary.
 */
template <typename T,
          template <class> class UnderflowPolicy = DefaultUnderflowPolicy>
class AtomicNonNegativeCounter : public UnderflowPolicy<T> {
    static_assert(
            std::is_unsigned<T>::value,
            "AtomicNonNegativeCounter should only be templated over unsigned types");

    using SignedT = typename std::make_signed<T>::type;

public:
    using value_type = T;

    AtomicNonNegativeCounter() = default;

    AtomicNonNegativeCounter(T initial) {
        store(initial);
    }

    AtomicNonNegativeCounter(const AtomicNonNegativeCounter& other) noexcept {
        store(other.load());
    }

    operator T() const noexcept {
        return load();
    }

    [[nodiscard]] T load() const noexcept {
        return value.load(std::memory_order_relaxed);
    }

    void store(T desired) {
        if (desired > T(std::numeric_limits<SignedT>::max())) {
            UnderflowPolicy<T>::underflow(desired, load(), desired);
        }
        value.store(desired, std::memory_order_relaxed);
    }

    /**
     * Add 'arg' to the current value. If the new value would underflow (i.e. if
     * arg was negative and current less than arg) then calls underflow() on the
     * selected UnderflowPolicy.
     *
     * Note: Not marked 'noexcept' as underflow() could throw.
     */
    T fetch_add(SignedT arg) {
        T current = load();
        T desired;
        do {
            if (arg < 0) {
                desired = current - T(std::abs(arg));
                if (SignedT(current) + arg < 0) {
                    UnderflowPolicy<T>::underflow(desired, current, arg);
                }
            } else {
                desired = current + T(arg);
                if (desired > T(std::numeric_limits<SignedT>::max())) {
                    UnderflowPolicy<T>::underflow(desired, current, arg);
                }
            }
            // Attempt to set the atomic value to desired. If the atomic value
            // is not the same as current then it has changed during
            // operation. compare_exchange_weak will reload the new value
            // into current if it fails, and we will retry.
        } while (!value.compare_exchange_weak(
                current, desired, std::memory_order_relaxed));

        return current;
    }

    /**
     * Subtract 'arg' from the current value. If the new value would underflow
     * then calls underflow() on the selected UnderflowPolicy.
     *
     * Note: Not marked 'noexcept' as underflow() could throw.
     */
    T fetch_sub(SignedT arg) {
        T current = load();
        T desired;
        do {
            if (arg < 0) {
                desired = current + T(std::abs(arg));
            } else {
                desired = current - T(arg);
            }

            if (desired > T(std::numeric_limits<SignedT>::max())) {
                UnderflowPolicy<T>::underflow(desired, current, arg);
            }
            // Attempt to set the atomic value to desired. If the atomic value
            // is not the same as current then it has changed during
            // operation. compare_exchange_weak will reload the new value
            // into current if it fails, and we will retry.
        } while (!value.compare_exchange_weak(
                current, desired, std::memory_order_relaxed));

        return current;
    }

    T exchange(T arg) noexcept {
        return value.exchange(arg, std::memory_order_relaxed);
    }

    AtomicNonNegativeCounter& operator=(const AtomicNonNegativeCounter& rhs) noexcept {
        value.store(rhs.load(), std::memory_order_relaxed);
        return *this;
    }

    AtomicNonNegativeCounter& operator+=(const T rhs) {
        fetch_add(rhs);
        return *this;
    }

    AtomicNonNegativeCounter& operator+=(const AtomicNonNegativeCounter& rhs) {
        fetch_add(rhs.load());
        return *this;
    }

    AtomicNonNegativeCounter& operator-=(const T rhs) {
        fetch_sub(rhs);
        return *this;
    }

    AtomicNonNegativeCounter& operator-=(const AtomicNonNegativeCounter& rhs) {
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

    AtomicNonNegativeCounter& operator=(T val) {
        store(val);
        return *this;
    }

private:
    std::atomic<T> value{0};
};

template <typename T>
T format_as(const NonNegativeCounter<T>& nnc) {
    return nnc.load();
}

#if CB_DEVELOPMENT_ASSERTS
template <typename T>
T format_as(const NonNegativeCounter<T, ClampAtZeroUnderflowPolicy>& nnc) {
    return nnc.load();
}
#endif

template <typename T>
T format_as(const AtomicNonNegativeCounter<T>& annc) {
    return annc.load();
}
template <typename T>
T format_as(
        const AtomicNonNegativeCounter<T, ClampAtZeroUnderflowPolicy>& annc) {
    return annc.load();
}

} // namespace cb
