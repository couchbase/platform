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

#include <gsl/gsl-lite.hpp>
#include <cstddef>
#include <functional>
#include <vector>

namespace cb {
/**
 * A simple ring buffer implementation using a vector as the underlying storage.
 * The buffer will overwrite old data when it reaches capacity.
 *
 * @tparam T The type of elements stored in the ring buffer
 */
template <typename T>
class RingBuffer {
public:
    /// Initialize the vector with the exact capacity upfront
    explicit RingBuffer(std::size_t capacity)
        : buffer(capacity), max_capacity(capacity) {
    }

    /// Add an element to the ring buffer. If the buffer is full, the oldest
    /// element will be overwritten.
    void push(T value) {
        if (max_capacity == 0) {
            return;
        }
        if (num_items == max_capacity) {
            head = (head + 1) % max_capacity; // Move head forward to overwrite
        } else {
            ++num_items;
        }

        buffer[tail] = std::move(value);
        tail = (tail + 1) % max_capacity; // Wrap around
    }

    /// Iterate over all elements in the ring buffer in order from oldest to
    /// newest, calling the provided callback for each element.
    void iterate(const std::function<void(const T&)>& callback) const {
        for (std::size_t i = 0; i < num_items; ++i) {
            std::size_t index = (head + i) % max_capacity;
            callback(buffer[index]);
        }
    }

    /// Returns true if the buffer is empty (contains no elements)
    [[nodiscard]] bool is_empty() const noexcept {
        return num_items == 0;
    }

    /// Returns true if the buffer is at capacity and the next push will
    /// overwrite the oldest element
    [[nodiscard]] bool is_full() const noexcept {
        return num_items == max_capacity;
    }

    /// Returns the current number of elements in the buffer (up to capacity)
    [[nodiscard]] std::size_t size() const noexcept {
        return num_items;
    }

    /// Returns the maximum number of elements the buffer can hold
    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return max_capacity;
    }

protected:
    /// The underlying storage for the ring buffer. The size of this vector is
    /// fixed to the capacity of the buffer, and elements are overwritten
    /// in-place
    std::vector<T> buffer;
    /// The index of the oldest element in the buffer (the "head" of the ring)
    /// and where we would start reading from when iterating. This moves forward
    /// when the buffer is full and we overwrite old data.
    std::size_t head = 0;
    /// The index where the next element will be written (the "tail" of the
    /// ring). This moves forward with each push and wraps around to the
    /// beginning of the buffer.
    std::size_t tail = 0;
    /// The current number of elements in the buffer. This is incremented with
    /// each push until it reaches capacity, at which point it stays at capacity
    /// and the head/tail indices manage the overwriting.
    std::size_t num_items = 0;
    /// The maximum number of elements the buffer can hold. This is set at
    /// construction and does not change. The buffer will overwrite old data
    /// when num_items reaches this capacity.
    const std::size_t max_capacity;
};
} // namespace cb
