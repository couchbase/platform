/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <cstddef>
#include <stdexcept>

namespace cb {
/**
 * RoundRobin iterator that cycles through elements in a container.
 *
 * Maintains an internal index and returns elements sequentially, wrapping
 * around to the beginning after reaching the end of the container.
 * The container must provide size() and operator[] (e.g., std::vector,
 * std::array).
 *
 * The container is stored as a reference, so it must outlive the RoundRobin
 * instance. This means that it'll reflect any changes to the container (e.g.,
 * adding/removing elements) as long as the container is not destroyed. However,
 * the behavior is undefined if the container is modified in a way that
 * invalidates references (e.g., resizing a vector) while the RoundRobin is in
 * use.
 *
 * @tparam Container The container type (e.g., std::vector<int>)
 */
template <typename Container>
class RoundRobin {
public:
    /**
     * Construct a RoundRobin iterator from a container reference.
     *
     * @param container The container to iterate through. Must outlive this
     * RoundRobin.
     */
    explicit RoundRobin(Container& container) : elements(container) {
    }

    /// Check if the container is empty.
    [[nodiscard]] auto empty() const noexcept {
        return elements.empty();
    }

    /**
     * Get the next element in round-robin order.
     *
     * Returns the element at the current index, then increments the index
     * and wraps around to zero if past the end.
     *
     * @return Reference to the next element
     */
    [[nodiscard]] decltype(auto) next() {
        not_empty_guard();
        return elements[index++ % elements.size()];
    }

    /**
     * Get the next element in round-robin order (const overload).
     *
     * Returns the element at the current index, then increments the index
     * and wraps around to zero if past the end.
     *
     * @return Reference to the next element
     */
    [[nodiscard]] decltype(auto) next() const {
        not_empty_guard();
        return elements[index++ % elements.size()];
    }

    /**
     * Get the number of elements in the container.
     *
     * @return Size of the container
     */
    [[nodiscard]] decltype(auto) size() const noexcept {
        return elements.size();
    }

private:
    void not_empty_guard() const {
        if (elements.empty()) {
            throw std::runtime_error("RoundRobin: container must not be empty");
        }
    }
    Container& elements;
    mutable std::size_t index = 0;
};
} // namespace cb
