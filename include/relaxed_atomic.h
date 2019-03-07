/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc.
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

namespace cb {

/**
 * The RelaxedAtomic class wraps std::atomic<> and operates with
 * relaxed memory ordering.
 */
template <typename T>
class RelaxedAtomic {
public:
    RelaxedAtomic() {
        store(0);
    }

    RelaxedAtomic(const T& initial) {
        store(initial);
    }

    explicit RelaxedAtomic(const RelaxedAtomic& other) {
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
        return value.fetch_sub(arg, std::memory_order_relaxed);
    }

    RelaxedAtomic& operator=(const RelaxedAtomic& rhs) {
        store(rhs.load());
        return *this;
    }

    RelaxedAtomic& operator+=(const T rhs) {
        fetch_add(rhs);
        return *this;
    }

    RelaxedAtomic& operator+=(const RelaxedAtomic& rhs) {
        fetch_add(rhs.load());
        return *this;
    }

    RelaxedAtomic& operator-=(const T rhs) {
        fetch_sub(rhs);
        return *this;
    }

    RelaxedAtomic& operator-=(const RelaxedAtomic& rhs) {
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
        return fetch_sub(1) - 1;
    }

    T operator--(int) {
        return fetch_sub(1);
    }

    RelaxedAtomic& operator=(T val) {
        store(val);
        return *this;
    }

    void reset() {
        store(0);
    }

    void setIfGreater(const T& val) {
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

    void setIfGreater(const RelaxedAtomic& val) {
        setIfGreater(val.load());
    }

    void setIfSmaller(const T& val) {
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

    void setIfSmaller(const RelaxedAtomic& val) {
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
    void setAdd(const T& val) {
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
    void setAdd(const RelaxedAtomic& val) {
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
    void setSub(const T& val) {
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
    void setSub(const RelaxedAtomic& val) {
        setSub(val.load());
    }

    bool compare_exchange_weak(T& expected, T desired) {
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
} // namespace cb
