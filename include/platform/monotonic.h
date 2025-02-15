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

#include <platform/atomic.h>
#include <platform/comparators.h>
#include <platform/exceptions.h>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeinfo>

/// Policy class for handling non-monotonic updates by simply ignoring them.
template <class T>
struct IgnorePolicy {
    void nonMonotonic(const T&, const T&, const std::string&) {
        // Ignore the update.
    }
};

/// Policy class for handling non-monotonic updates by throwing std::logic_error
template <class T>
struct ThrowExceptionPolicy {
    void nonMonotonic(const T& curValue,
                      const T& newValue,
                      const std::string& label) {
        using namespace std;
        cb::throwWithTrace(logic_error(
                string("Monotonic<") + typeid(T).name() + "> (" + label +
                ") invariant failed: new value (" + to_string(newValue) +
                ") breaks invariant on current value (" + to_string(curValue) +
                ")"));
    }
};

class BasicNameLabelPolicy {
public:
    std::string getLabel(const char* name) const {
        if (name) {
            return {name};
        }
        return "unlabelled";
    };
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

// We want EBO to trigger for the Monotonic classes. On MSVC, this requires
// an explicit declspec to opt into EBO for multiple empty bases.
#ifdef _MSC_VER
#define CB_MONOTONIC_EBO __declspec(empty_bases)
#else
#define CB_MONOTONIC_EBO
#endif // defined(_MSC_VER)

/**
 * Monotonic is a class template for simple types T. It provides guarantee
 * of updating the class objects by only values that are greater than what is
 * contained at the time of update - i.e. the object must be
 * Strictly Increasing.
 *
 * Note: This is not atomic/thread-safe. If you need thread-safely, see
 * AtomicMonotonic.
 *
 * Note 2: We privately inherit from the LabelPolicy to allow for EBO.
 *
 * @tparam T value type used to represent the value.
 * @tparam OrderReversePolicy Policy class which controls the behaviour if
 *         an operation would break the monotonic invariant.
 * @tparam Name a pointer to a string literal that stores the name of the
 *         variable name given to this Monotonic
 * @tparam LabelPolicy A class that proves a function getLabel() which
 *         takes a cont char* pointing to the Name and returns a label as
 *         std::string about the Monotnic<> in question
 * @tparam Invariant The invariant to maintain across updates.
 */
template <typename T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          const char* Name = nullptr,
          class LabelPolicy = BasicNameLabelPolicy,
          template <class> class Invariant = cb::greater>
class CB_MONOTONIC_EBO Monotonic : public OrderReversedPolicy<T>,
                                   private LabelPolicy {
public:
    using value_type = T;
    using BaseType = OrderReversedPolicy<T>;

    explicit Monotonic(const T val = std::numeric_limits<T>::min(),
                       LabelPolicy labeler = {})
        : LabelPolicy(labeler), val(val) {
    }

    Monotonic(const Monotonic& other)
        : LabelPolicy(static_cast<const LabelPolicy&>(other)), val(other.val) {
    }

    Monotonic& operator=(const Monotonic& other) {
        if (Invariant<T>()(other.val, val)) {
            val = other.val;
        } else {
            const char* name = Name;
            BaseType::nonMonotonic(val, other.val, LabelPolicy::getLabel(name));
        }
        return *this;
    }

    Monotonic& operator=(const T& v) {
        if (Invariant<T>()(v, val)) {
            val = v;
        } else {
            const char* name = Name;
            BaseType::nonMonotonic(val, v, LabelPolicy::getLabel(name));
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

    void setLabeler(LabelPolicy newLabeler) {
        LabelPolicy::operator=(std::move(newLabeler));
    };

private:
    T val;
};

static_assert(sizeof(Monotonic<int, IgnorePolicy>) == sizeof(int),
              "Expected EBO to kick in.");

/**
 * A weakly increasing template type (allows the same existing value to be
 * stored).
 */
template <class T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          const char* Name = nullptr,
          class LabelFactory = BasicNameLabelPolicy>
using WeaklyMonotonic = Monotonic<T,
                                  OrderReversedPolicy,
                                  Name,
                                  LabelFactory,
                                  cb::greater_equal>;

/**
 * Variant of the Monotonic class, except that the type T is wrapped in
 * std::atomic, so all updates are atomic. T must be TriviallyCopyable.
 *
 * Note: We privately inherit from the LabelPolicy to allow for EBO.
 */
template <typename T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          const char* Name = nullptr,
          class LabelPolicy = BasicNameLabelPolicy,
          template <class> class Invariant = cb::greater>
class CB_MONOTONIC_EBO AtomicMonotonic : public OrderReversedPolicy<T>,
                                         private LabelPolicy {
public:
    explicit AtomicMonotonic(T val = std::numeric_limits<T>::min(),
                             LabelPolicy labeler = {})
        : LabelPolicy(std::move(labeler)), val(val) {
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
                const char* name = Name;
                OrderReversedPolicy<T>::nonMonotonic(
                        current, desired, LabelPolicy::getLabel(name));
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

    void setLabeler(LabelPolicy newLabeler) {
        LabelPolicy::operator=(std::move(newLabeler));
    };

private:
    std::atomic<T> val;
};

static_assert(sizeof(AtomicMonotonic<int, IgnorePolicy>) == sizeof(int),
              "Expected EBO to kick in.");

#undef CB_MONOTONIC_EBO

/**
 * A weakly increasing atomic template type (allows the same existing value to
 * be stored).
 */
template <class T,
          template <class> class OrderReversedPolicy =
                  DefaultOrderReversedPolicy,
          const char* Name = nullptr,
          class LabelFactory = BasicNameLabelPolicy>
using AtomicWeaklyMonotonic = AtomicMonotonic<T,
                                              OrderReversedPolicy,
                                              Name,
                                              LabelFactory,
                                              cb::greater_equal>;

#define BASE_MONOTONIC(NAME, TYPE, POLICY, LABELER, VARNAME) \
    /* NOLINTNEXTLINE(modernize-avoid-c-arrays) */           \
    constexpr static const char VARNAME##Label[] = #VARNAME; \
    NAME<TYPE, POLICY, VARNAME##Label, LABELER> VARNAME

#define MONOTONIC2(TYPE, NAME) MONOTONIC3(TYPE, NAME, BasicNameLabelPolicy)
#define MONOTONIC3(TYPE, NAME, LABELER) \
    MONOTONIC4(TYPE, NAME, LABELER, DefaultOrderReversedPolicy)
#define MONOTONIC4(TYPE, NAME, LABELER, POLICY) \
    BASE_MONOTONIC(Monotonic, TYPE, POLICY, LABELER, NAME)

#define WEAKLY_MONOTONIC2(TYPE, NAME) \
    WEAKLY_MONOTONIC3(TYPE, NAME, BasicNameLabelPolicy)
#define WEAKLY_MONOTONIC3(TYPE, NAME, LABELER) \
    WEAKLY_MONOTONIC4(TYPE, NAME, LABELER, DefaultOrderReversedPolicy)
#define WEAKLY_MONOTONIC4(TYPE, NAME, LABELER, POLICY) \
    BASE_MONOTONIC(WeaklyMonotonic, TYPE, POLICY, LABELER, NAME)

#define ATOMIC_MONOTONIC2(TYPE, NAME) \
    ATOMIC_MONOTONIC3(TYPE, NAME, BasicNameLabelPolicy)
#define ATOMIC_MONOTONIC3(TYPE, NAME, LABELER) \
    ATOMIC_MONOTONIC4(TYPE, NAME, LABELER, DefaultOrderReversedPolicy)
#define ATOMIC_MONOTONIC4(TYPE, NAME, LABELER, POLICY) \
    BASE_MONOTONIC(AtomicMonotonic, TYPE, POLICY, LABELER, NAME)

#define ATOMIC_WEAKLY_MONOTONIC2(TYPE, NAME) \
    ATOMIC_WEAKLY_MONOTONIC3(TYPE, NAME, BasicNameLabelPolicy)
#define ATOMIC_WEAKLY_MONOTONIC3(TYPE, NAME, LABELER) \
    ATOMIC_WEAKLY_MONOTONIC4(TYPE, NAME, LABELER, DefaultOrderReversedPolicy)
#define ATOMIC_WEAKLY_MONOTONIC4(TYPE, NAME, LABELER, POLICY) \
    BASE_MONOTONIC(AtomicWeaklyMonotonic, TYPE, POLICY, LABELER, NAME)

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
