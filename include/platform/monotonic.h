/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2021-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <platform/comparators.h>

#include <platform/atomic.h>
#include <platform/exceptions.h>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeinfo>

/// Policy class for handling non-monotonic updates by simply ignoring them.
template <class T>
struct IgnorePolicy {
    void nonMonotonic(const T&, const T&) {
        // Ignore the update.
    }

    void setLabel(const std::string&) {
        // There is no label on the IgnorePolicy
    }
};

/// Policy class for handling non-monotonic updates by throwing std::logic_error
template <class T>
struct ThrowExceptionPolicy {
    ThrowExceptionPolicy() = default;
    ThrowExceptionPolicy(const ThrowExceptionPolicy& other)
        : label(other.label) {
    }

    void nonMonotonic(const T& curValue, const T& newValue) {
        using std::to_string;
        cb::throwWithTrace(std::logic_error(
                std::string("Monotonic<") + typeid(T).name() + "> (" + label +
                ") invariant failed: new value (" + to_string(newValue) +
                ") breaks invariant on current value (" + to_string(curValue) +
                ")"));
    }

    /**
     * Set the label to give this monotonic value. Used in nonMonotonic() to
     * give a more descriptive exception message to aid debugging.
     */
    void setLabel(const std::string& newLabel) {
        this->label = newLabel;
    }

private:
    std::string label{"unlabelled"};
};

// Default Monotonic OrderReversedPolicy (if user doesn't explicitly
// specify otherwise) use IgnorePolicy for Release builds,
// and ThrowExceptionPolicy for Pre-Release builds.
template <class T>
#if CB_DEVELOPMENT_ASSERTS
using DefaultOrderReversedPolicy = ThrowExceptionPolicy<T>;
#else
using DefaultOrderReversedPolicy = IgnorePolicy<T>;
#endif

/**
 * Monotonic is a class template for simple types T. It provides guarantee
 * of updating the class objects by only values that are greater than what is
 * contained at the time of update - i.e. the object must be
 * Strictly Increasing.
 *
 * Note: This is not atomic/thread-safe. If you need thread-safely, see
 * AtomicMonotonic.
 *
 * @tparam T value type used to represent the value.
 * @tparam OrderReversePolicy Policy class which controls the behaviour if
 *         an operation would break the monotonic invariant.
 * @tparam Invariant The invariant to maintain across updates.
 */
template <typename T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          template <class> class Invariant = cb::greater>
class Monotonic : public OrderReversedPolicy<T> {
public:
    using value_type = T;

    explicit Monotonic(const T val = std::numeric_limits<T>::min()) : val(val) {
    }

    Monotonic(const Monotonic& other)
        : OrderReversedPolicy<T>(other), val(other.val) {
    }

    Monotonic& operator=(const Monotonic& other) {
        if (Invariant<T>()(other.val, val)) {
            val = other.val;
        } else {
            OrderReversedPolicy<T>::nonMonotonic(val, other.val);
        }
        return *this;
    }

    Monotonic& operator=(const T& v) {
        if (Invariant<T>()(v, val)) {
            val = v;
        } else {
            OrderReversedPolicy<T>::nonMonotonic(val, v);
        }
        return *this;
    }

    // Add no lint to allow implicit casting back to the value_type of the
    // template.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T() const noexcept {
        return load();
    }

    [[nodiscard]] T load() const noexcept {
        return val;
    }

    T operator++() {
        operator=(val + 1);
        return val;
    }

    T operator++(int) {
        T old = val;
        operator=(val + 1);
        return old;
    }

    T operator+=(T rhs) {
        return val += rhs;
    }

    /* Can be used to lower the value */
    void reset(T desired) {
        val = desired;
    }

private:
    T val;
};

/**
 * A weakly increasing template type (allows the same existing value to be
 * stored).
 */
template <class T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy>
using WeaklyMonotonic = Monotonic<T, OrderReversedPolicy, cb::greater_equal>;

/**
 * Variant of the Monotonic class, except that the type T is wrapped in
 * std::atomic, so all updates are atomic. T must be TriviallyCopyable.
 */
template <typename T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          template <class> class Invariant = cb::greater>
class AtomicMonotonic : public OrderReversedPolicy<T> {
public:
    explicit AtomicMonotonic(T val = std::numeric_limits<T>::min()) : val(val) {
    }

    AtomicMonotonic(const AtomicMonotonic<T>& other) = delete;

    AtomicMonotonic(AtomicMonotonic<T>&& other) = delete;

    AtomicMonotonic& store(
            T desired,
            std::memory_order memoryOrder = std::memory_order_seq_cst) {
        while (true) {
            T current = val.load(memoryOrder);
            if (Invariant<T>()(desired, current)) {
                if (val.compare_exchange_weak(
                            current, desired, memoryOrder, memoryOrder)) {
                    break;
                }
            } else {
                OrderReversedPolicy<T>::nonMonotonic(current, desired);
                break;
            }
        }
        return *this;
    }

    /**
     * store desired only if it is bigger than the current value.
     */
    AtomicMonotonic& storeIfBigger(T desired) {
        atomic_setIfBigger(val, desired);
        return *this;
    }

    AtomicMonotonic& operator=(T desired) {
        return store(desired);
    }

    // Add no lint to allow implicit casting back to the value_type of the
    // template.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T() const {
        return val;
    }

    T operator++() {
        if (Invariant<T>()(val + 1, val)) {
            return ++val;
        }
        return val;
    }

    T operator++(int) {
        if (Invariant<T>()(val + 1, val)) {
            return val++;
        }
        return val;
    }

    T load(std::memory_order memoryOrder = std::memory_order_seq_cst) const {
        return val.load(memoryOrder);
    }

    /* Can be used to lower the value */
    void reset(T desired,
               std::memory_order memoryOrder = std::memory_order_seq_cst) {
        val.store(desired, memoryOrder);
    }

private:
    std::atomic<T> val;
};

/**
 * A weakly increasing atomic template type (allows the same existing value to
 * be stored).
 */
template <class T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy>
using AtomicWeaklyMonotonic =
        AtomicMonotonic<T, OrderReversedPolicy, cb::greater_equal>;

template <typename T>
T format_as(const WeaklyMonotonic<T>& t) {
    return t.load();
}
template <typename T>
T format_as(const Monotonic<T>& t) {
    return t.load();
}
template <typename T>
T format_as(const AtomicMonotonic<T>& t) {
    return t.load();
}
template <typename T>
T format_as(const AtomicWeaklyMonotonic<T>& t) {
    return t.load();
}
