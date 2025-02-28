/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <folly/ScopeGuard.h>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cb {

/**
 * Used as a tag to specify that the GuardHolder should be created without a
 * guard in place.
 */
struct DeferGuard {};

/**
 * std::unique_lock equivalent for a scope guard object.
 * GuardHolder creates the scope guard at construction.
 * Calling reset() ("unique_lock::unlock") destroys it, and calling emplace
 * ("unique_lock::lock") re-creates it.
 */
template <typename Guard, typename... Args>
class GuardHolder {
public:
    /**
     * Create the GuardHolder and create a scope guard.
     */
    template <typename... Ts>
    GuardHolder(Ts&&... ts) : args(std::make_tuple(std::forward<Ts&&>(ts)...)) {
        emplace();
    }

    /**
     * Create the GuardHolder without a scope guard in place.
     */
    template <typename... Ts>
    GuardHolder(DeferGuard, Ts&&... ts)
        : args(std::make_tuple(std::forward<Ts&&>(ts)...)) {
    }

    GuardHolder(const GuardHolder& other) = default;
    GuardHolder(GuardHolder&&) = default;
    GuardHolder& operator=(const GuardHolder& other) = default;
    GuardHolder& operator=(GuardHolder&&) = default;

    /**
     * Explicit conversion to boolean.
     * @returns true if the scope guard exists
     */
    explicit operator bool() const {
        return static_cast<bool>(guard);
    }

    /**
     * Re-creates the scope guard.
     */
    void emplace() {
        std::apply(
                [this](auto&&... ts) {
                    guard.emplace(std::forward<decltype(ts)>(ts)...);
                },
                args);
    }

    /**
     * Destroys the current scope guard.
     */
    void reset() {
        guard.reset();
    }

private:
    // The current scope guard.
    std::optional<Guard> guard{};
    /// Args used to re-create the scope guard.
    std::tuple<Args...> args;
};

/**
 * Guards a piece of data using a scope guard and only allows access to that
 * data under that guard. The data is always constructed / destroyed under that
 * guard as well.
 *
 * The data can be accessed by calling withGuard(), similar to withLock() when
 * using folly::Synchronized.
 *
 * The intention is to use this class with something like cb::NoArenaGuard.
 * Example:
 *  Guarded<std::unique_ptr<int>, NoArenaGuard> ptr{nullptr};
 */
template <typename T, typename Guard, typename... GuardArgs>
class Guarded {
public:
    template <typename... TArgs>
    Guarded(TArgs&&... targs) : gh{}, object{std::forward<TArgs&&>(targs)...} {
        gh.reset();
    }

    template <typename... TArgs, typename... GArgs>
    Guarded(std::piecewise_construct_t,
            std::tuple<TArgs...> targs,
            std::tuple<GArgs...> gargs)
        : Guarded{std::piecewise_construct,
                  std::move(targs),
                  std::move(gargs),
                  std::make_index_sequence<sizeof...(TArgs)>{},
                  std::make_index_sequence<sizeof...(GArgs)>{}} {
        gh.reset();
    }

    Guarded(const Guarded& other)
        : Guarded{std::piecewise_construct, other.gh, other.object} {
    }

    Guarded(Guarded&& other)
        : Guarded{std::piecewise_construct,
                  std::move(other.gh),
                  std::move(other.object)} {
    }

    Guarded& operator=(const Guarded&) = default;
    Guarded& operator=(Guarded&&) = default;

    /// Assign the contained object under a scope guard.
    Guarded& operator=(const T& other) {
        Expects(!gh);
        auto g = folly::makeGuard([this]() { gh.reset(); });
        gh.emplace();
        object = other;
        return *this;
    }

    /// Move-assign the contained object under a scope guard.
    Guarded& operator=(T&& other) {
        Expects(!gh);
        auto g = folly::makeGuard([this]() { gh.reset(); });
        gh.emplace();
        object = std::move(other);
        return *this;
    }

    ~Guarded() {
        Expects(!gh);
        gh.emplace();
    }

    /// Access to the object without scope guard.
    T& getUnsafe() {
        return object;
    }

    /// Access to the object without scope guard (const).
    const T& getUnsafe() const {
        return object;
    }

    /// Access the object with a scope guard.
    template <typename F>
    auto withGuard(F&& f) {
        Expects(!gh);
        auto g = folly::makeGuard([this]() { gh.reset(); });
        gh.emplace();
        return f(object);
    }

    /// Access the object with a scope guard (const).
    template <typename F>
    auto withGuard(F&& f) const {
        Expects(!gh);
        auto g = folly::makeGuard([this]() { gh.reset(); });
        gh.emplace();
        return f(object);
    }

private:
    template <typename... TArgs,
              typename... GArgs,
              size_t... TIs,
              size_t... GIs>
    Guarded(std::piecewise_construct_t,
            std::tuple<TArgs...> targs,
            std::tuple<GArgs...> gargs,
            std::index_sequence<TIs...>,
            std::index_sequence<GIs...>)
        : gh{std::move(std::get<GIs>(gargs))...},
          object{std::move(std::get<TIs>(targs))...} {
    }

    GuardHolder<Guard, std::decay_t<GuardArgs>...> gh;
    T object;
};

/**
 * Create a Guarded object. Executes the constructor function under the scope
 * guard. Any additional arguments are passed to the scope guard holder.
 * @param construct the function used to create the object
 * @param gargs arguments to pass to the scope guard holder
 */
template <typename Guard, typename F, typename... GuardArgs>
Guarded<std::invoke_result_t<F>, Guard, GuardArgs...> makeGuarded(
        F&& construct, const GuardArgs&... gargs) {
    GuardHolder<Guard, GuardArgs...> gh(gargs...);
    return {std::piecewise_construct,
            std::make_tuple<std::invoke_result_t<F>&&>(construct()),
            std::make_tuple(DeferGuard{}, gargs...)};
}

} // namespace cb
