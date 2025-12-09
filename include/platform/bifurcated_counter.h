/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <algorithm>
#include <atomic>
#include <type_traits>

namespace cb {

namespace detail {

/**
 * Type trait for types with a value_type member typedef.
 * value_type<T>::type is T::value_type if it exists, T otherwise.
 */
template <typename T, typename = void>
struct value_type {
    using type = T;
};

/**
 * Specialisation used for types with a value_type member typedef.
 */
template <typename T>
struct value_type<T, std::void_t<typename T::value_type>> {
    using type = typename T::value_type;
};

/**
 * Specialisation for std::atomic<T>. Should be covered by the above
 * specialisation for types with value_type, but old AppleClang on CV
 * doesn't have value_type on std::atomic specialisations.
 */
template <typename T>
struct value_type<std::atomic<T>, void> {
    using type = T;
};

/**
 * We could have just inserted the decltype() from above here and done away
 * with the struct, but MSVC 2017 doesn't like it :(
 */
template <typename T>
using value_type_t = typename value_type<T>::type;

static_assert(std::is_same_v<int, value_type_t<int>>);
static_assert(std::is_same_v<int, value_type_t<std::atomic<int>>>);

} // namespace detail

/**
 * A counter which maintains separate added/removed counts.
 * @tparam T Signed counter type which can optionally be an atomic.
 */
template <typename T>
struct BifurcatedCounter {
    /**
     * If T has a nested value_type typename, use that as the arithmetic type.
     * This allows T to be an arithmetic, std::atomic or cb::RelaxedAtomic type,
     * without requiring specialisations of this counter template for each.
     */
    using value_type = detail::value_type_t<T>;
    static_assert(std::is_signed_v<value_type>,
                  "A signed integer type is required.");

    BifurcatedCounter() = default;

    BifurcatedCounter(const BifurcatedCounter&) = default;
    BifurcatedCounter& operator=(const BifurcatedCounter&) = default;

    /**
     * Allow conversion from a counter with a value type which is convertible
     * to this value type.
     *
     * Example: BifurcatedCounter<std::atomic<int>> -> BifurcatedCounter<int>
     */
    template <typename U,
              typename = std::enable_if_t<
                      std::is_convertible_v<detail::value_type_t<U>, T>>>
    explicit BifurcatedCounter(BifurcatedCounter<U> other)
        : added(other.added), removed(other.removed) {
    }

    /**
     * Note because added and removed counts are maintained separately and not
     * synchronised, the returned value might be negative.
     *
     * Use the loadNonNegative() method to clamp the minimum value to 0.
     */
    [[nodiscard]] value_type load() const {
        return static_cast<value_type>(added) -
               static_cast<value_type>(removed);
    }

    [[nodiscard]] value_type loadNonNegative() const {
        return std::max(value_type{}, load());
    }

    operator value_type() const {
        return load();
    }

    BifurcatedCounter& operator+=(value_type arg) {
        if (arg >= 0) {
            added += arg;
        } else {
            removed -= arg;
        }
        return *this;
    }

    template <typename U,
              typename = std::enable_if_t<
                      std::is_convertible_v<detail::value_type_t<U>, T>>>
    BifurcatedCounter& operator+=(const BifurcatedCounter<U>& arg) {
        added += static_cast<U>(arg.added);
        removed += static_cast<U>(arg.removed);
        return *this;
    }

    BifurcatedCounter& operator-=(value_type arg) {
        *this += -arg;
        return *this;
    }

    template <typename U,
              typename = std::enable_if_t<
                      std::is_convertible_v<detail::value_type_t<U>, T>>>
    BifurcatedCounter& operator-=(const BifurcatedCounter<U>& arg) {
        added -= static_cast<U>(arg.added);
        removed -= static_cast<U>(arg.removed);
        return *this;
    }

    value_type operator++() {
        ++added;
        return load();
    }

    value_type operator++(int) {
        ++added;
        return load() - 1;
    }

    value_type operator--() {
        ++removed;
        return load();
    }

    value_type operator--(int) {
        ++removed;
        return load() + 1;
    }

    value_type getAdded() const {
        return added;
    }

    value_type getRemoved() const {
        return removed;
    }

    /**
     * Resets the counter values to 0.
     *
     * This is not an atomic operation even when T is atomic. This means that
     * during a reset(), an observer could see (A -> A' -> 0).
     */
    void reset() {
        added = {};
        removed = {};
    }

private:
    template <typename>
    friend struct BifurcatedCounter;

    T added{};
    T removed{};
};

} // namespace cb
