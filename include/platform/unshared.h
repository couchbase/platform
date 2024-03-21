/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <folly/lang/Aligned.h>
#include <gsl/gsl-lite.hpp>
#include <platform/corestore.h>
#include <relaxed_atomic.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace cb {

/**
 * Enumeration used as the default Index template parameter of Unshared<Index,
 * Count, Integer>. Allows only a single value to be stored.
 */
enum class MonoIndex : std::size_t {
    Default = 0,
    Count = 1,
};

/**
 * A high-performance counter (or set of counters) optimised to reduce
 * cross-cache communication.
 *
 * The counters can be read in two modes:
 * - estimate (off by up to NUM_CPUS x threshold)
 * - precise (which updates the estimate as a side effect)
 *
 * Every update to these counters is stored as a delta in a core-local counter.
 * Whenever the core-local delta goes above the configured threshold, the delta
 * is cleared and the estiamte is updated with it.
 *
 * The precise read works by clearing all of the deltas, and updating the
 * estimate. Since clearing the deltas is not a single atomic operation, the
 * precise value is still not a point-in-time snapshot.
 *
 * Additionally, an interleaved sequence of Decrement(Core-1), GetPrecise,
 * Increment(Core-2) could leave the internal estimate negative.
 *
 * To fix this, any negative values are capped to 0 when they are returned to
 * the caller and the caller will never observe the negative values.
 *
 * @tparam Index an enumeration type which can be used to read the stored value.
 * If this type has a Default member, that will be used as a default parameter
 * to some methods.
 * @tparam Count the number of values that should be stored. If the enumeration
 * has a Count enumerator, it's integer value is used as the count.
 * @tparam Integer the signed integer type to use for the counters.
 * @tparam CountFn passed down to CoreStore.
 * @tparam IndexFn passed down to CoreStore.
 */
template <typename Index,
          std::size_t Count = std::size_t(Index::Count),
          typename Integer = std::int64_t,
          CountFnType CountFn = cb::get_cpu_count,
          IndexFnType IndexFn = cb::stripe_for_current_cpu>
struct Unshared {
public:
    static_assert(std::is_enum_v<Index>, "Index must be an enumeration");
    static_assert(Count, "Count must be greater than 0");
    static_assert(std::is_signed_v<Integer>,
                  "Integer must be a signed integer type");

    /**
     * Set the threshold for the maximum core-local delta that is allowed,
     * before we have to update the estimate.
     */
    void setCoreThreshold(Integer value) {
        coreThreshold = value;
    }

    /**
     * Get the threshold for the maximum core-local delta.
     */
    Integer getCoreThreshold() const {
        return coreThreshold;
    }

    /**
     * Returns the maximum difference that can be observed between an estimate
     * read and a precise read.
     */
    Integer getMaximumDrift() const {
        return coreThreshold * static_cast<Integer>(CountFn());
    }

    /**
     * Performs arithmetic addition on the counter.
     * This may update the estiamte, if it causes the core-local delta to go
     * above the allowed threshold.
     */
    void add(Integer value, Index index = Index::Default) {
        const auto i = static_cast<std::size_t>(index);
        auto& delta = (*coreDeltas.get())[i];
        auto newDelta = delta.fetch_add(value) + value;

        // Check if we need to update the estimates.
        if (std::abs(newDelta) > coreThreshold) {
            auto clearedDelta = delta.exchange(0);
            estimates[i].fetch_add(clearedDelta);
            if constexpr (Count > 1) {
                sumOfEstimates.fetch_add(clearedDelta);
            }
        }
    }

    /**
     * Performs arithmetic subtraction on the counter.
     * This may update the estiamte, if it causes the core-local delta to go
     * above the allowed threshold.
     */
    void sub(Integer value, Index index = Index::Default) {
        return add(-value, index);
    }

    /**
     * Reads the current estimate for the element at the given index.
     * If Index=MonoIndex, this function can be called without parameters.
     * NOTE: If the Index type is MonoIndex, this function can be called without
     * parameters.
     * @return the current estimate
     */
    Integer getEstimate(Index index = Index::Default) const {
        return std::max(Integer{},
                        estimates[static_cast<std::size_t>(index)].load());
    }

    /**
     * Updates the estimate for the element at the given index, by clearing all
     * CoreLocal deltas, and returns the new estimate.
     * NOTE: If the Index type is MonoIndex, this function can be called without
     * parameters.
     * @return the current estimate
     */
    Integer getPrecise(Index index = Index::Default) const {
        const auto i = static_cast<std::size_t>(index);
        // We will go over all the core-locals and update the estimate with the
        // deltas. Keeping the last returned value from estimates[i].fetch_add
        // means we don't have to have a separate estimate[i].load() at the
        // function return, as we can just return this.
        Integer latestEstimate{};
        for (auto& core : coreDeltas) {
            auto& deltas = *core.get();
            auto clearedDelta = deltas[i].exchange(0);
            latestEstimate =
                    estimates[i].fetch_add(clearedDelta) + clearedDelta;
            if constexpr (Count > 1) {
                sumOfEstimates.fetch_add(clearedDelta);
            }
        }
        return std::max(Integer{}, latestEstimate);
    }

    /**
     * Returns the sum of the current estimates, using a single atomic load
     * instruction.
     * @return the current estimate
     */
    Integer getEstimateSum() const {
        if constexpr (Count > 1) {
            return std::max(Integer{}, sumOfEstimates.load());
        }
        return std::max(Integer{}, estimates[0].load());
    }

    /**
     * Updates the estimates of all elements, by clearing the core-local deltas,
     * and returns the sum of all estimates after the update.
     * @return the sum of the updated estimates
     */
    Integer getPreciseSum() const {
        Integer sum{};
        for (std::size_t i = 0; i < Count; i++) {
            sum += getPrecise(static_cast<Index>(i));
        }
        return std::max(Integer{}, sum);
    }

    /**
     * Zeroes everything, except the threshold.
     */
    void reset() {
        for (auto& core : coreDeltas) {
            auto& deltas = *core.get();
            std::fill(deltas.begin(), deltas.end(), Integer{});
        }
        std::fill(estimates.begin(), estimates.end(), Integer{});
        sumOfEstimates.reset();
    }

private:
    /**
     * Storage for the require number of counters.
     */
    using Array = std::array<cb::RelaxedAtomic<Integer>, Count>;

    /**
     * The core-local deltas of all counters.
     * Marked mutable so we can getPrecise on a const object while also updating
     * the estimates.
     */
    mutable CoreStore<folly::cacheline_aligned<Array>, CountFn, IndexFn>
            coreDeltas{};
    /**
     * The current estimates of all counters.
     * Mutable for the reason given above.
     */
    mutable Array estimates{};
    /**
     * The sum of all estimates.
     * Mutable for the reason given above.
     * NOTE: This member is unused when the number of counters is 1.
     */
    mutable cb::RelaxedAtomic<Integer> sumOfEstimates{};
    /**
     * The threshold for the core-local deltas for all counters.
     */
    cb::RelaxedAtomic<Integer> coreThreshold{};
};

/**
 * A variant of Unshared with only a single value.
 */
template <typename Integer = std::int64_t>
using MonoUnshared = Unshared<MonoIndex, 1, Integer>;

} // namespace cb
