/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <atomic>

namespace cb {

/**
 * The RelaxedAtomic class wraps std::atomic<> and operates with
 * relaxed memory ordering.
 */
template <typename T>
class RelaxedAtomic {
public:
    using value_type = T;

    RelaxedAtomic() noexcept {
        store({});
    }

    RelaxedAtomic(const T& initial) noexcept {
        store(initial);
    }

    explicit RelaxedAtomic(const RelaxedAtomic& other) noexcept {
        store(other.load());
    }

    operator T() const noexcept {
        return load();
    }

    [[nodiscard]] T load() const noexcept {
        return value.load(std::memory_order_relaxed);
    }

    void store(T desired) noexcept {
        value.store(desired, std::memory_order_relaxed);
    }

    T fetch_add(T arg) noexcept {
        return value.fetch_add(arg, std::memory_order_relaxed);
    }

    T fetch_sub(T arg) noexcept {
        return value.fetch_sub(arg, std::memory_order_relaxed);
    }

    RelaxedAtomic& operator=(const RelaxedAtomic& rhs) noexcept {
        store(rhs.load());
        return *this;
    }

    RelaxedAtomic& operator+=(const T rhs) noexcept {
        fetch_add(rhs);
        return *this;
    }

    RelaxedAtomic& operator+=(const RelaxedAtomic& rhs) noexcept {
        fetch_add(rhs.load());
        return *this;
    }

    RelaxedAtomic& operator-=(const T rhs) noexcept {
        fetch_sub(rhs);
        return *this;
    }

    RelaxedAtomic& operator-=(const RelaxedAtomic& rhs) noexcept {
        fetch_sub(rhs.load());
        return *this;
    }

    T operator++() noexcept {
        return fetch_add(1) + 1;
    }

    T operator++(int) noexcept {
        return fetch_add(1);
    }

    T operator--() noexcept {
        return fetch_sub(1) - 1;
    }

    T operator--(int) noexcept {
        return fetch_sub(1);
    }

    RelaxedAtomic& operator=(T val) noexcept {
        store(val);
        return *this;
    }

    void reset() noexcept {
        store({});
    }

    void setIfGreater(const T& val) noexcept {
        do {
            T currval = load();
            if (val > currval) {
                if (value.compare_exchange_weak(
                            currval, val, std::memory_order_relaxed)) {
                    break;
                }
            } else {
                break;
            }
        } while (true);
    }

    void setIfGreater(const RelaxedAtomic& val) noexcept {
        setIfGreater(val.load());
    }

    void setIfSmaller(const T& val) noexcept {
        do {
            T currval = load();
            if (val < currval) {
                if (value.compare_exchange_weak(
                            currval, val, std::memory_order_relaxed)) {
                    break;
                }
            } else {
                break;
            }
        } while (true);
    }

    void setIfSmaller(const RelaxedAtomic& val) noexcept {
        setIfSmaller(val.load());
    }

    /**
     * Implementation similar to fetch_add. This function is needed as not
     * all implementations of RelaxedAtomic are able to make use of the
     * operator overload methods if they are not of integral type. In the
     * first case, using the operator overload is preferred as it is more
     * optimized, this should only be used if that is not possible.
     * @param val The value to add to that of the one stored
     */
    void setAdd(const T& val) noexcept {
        do {
            T currval = load();
            if (value.compare_exchange_weak(
                        currval, currval + val, std::memory_order_relaxed)) {
                break;
            }
        } while (true);
    }

    /**
     * Implementation similar to fetch_add. This function is needed as not
     * all implementations of RelaxedAtomic are able to make use of the
     * operator overload methods if they are not of integral type. In the
     * first case, using the operator overload is preferred as it is more
     * optimized, this should only be used if that is not possible.
     * @param val The RelaxedAtomic containing the value to add to that of
     * the one stored
     */
    void setAdd(const RelaxedAtomic& val) noexcept {
        setAdd(val.load());
    }

    /**
     * Implementation similar to fetch_sub. This function is needed as not
     * all implementations of RelaxedAtomic are able to make use of the
     * operator overload methods if they are not of integral type. In the
     * first case, using the operator overload is preferred as it is more
     * optimized, this should only be used if that is not possible.
     * @param val The value to subtract to that of the one stored
     */
    void setSub(const T& val) noexcept {
        do {
            T currval = load();
            if (value.compare_exchange_weak(
                        currval, currval - val, std::memory_order_relaxed)) {
                break;
            }
        } while (true);
    }

    /**
     * Implementation similar to fetch_sub. This function is needed as not
     * all implementations of RelaxedAtomic are able to make use of the
     * operator overload methods if they are not of integral type. In the
     * first case, using the operator overload is preferred as it is more
     * optimized, this should only be used if that is not possible.
     * @param val The RelaxedAtomic containing the value to subtract to that
     * of the one stored
     */
    void setSub(const RelaxedAtomic& val) noexcept {
        setSub(val.load());
    }

    bool compare_exchange_weak(T& expected, T desired) noexcept {
        return value.compare_exchange_weak(expected,
                                           desired,
                                           std::memory_order_release,
                                           std::memory_order_relaxed);
    }

    T exchange(T desired) {
        return value.exchange(desired, std::memory_order_relaxed);
    }

private:
    std::atomic<T> value;
};

template <typename T>
T format_as(const RelaxedAtomic<T>& ra) {
    return ra.load();
}

} // namespace cb
