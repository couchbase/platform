/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <array>
#include <vector>

namespace cb {

/**
 * Template parameters
 * @param container_type the actual backing container, e.g. `std::vector<int>`
 * @param value_type the value type held by the container, e.g. `int`
 *
 * The rest of this class is relatively unexciting. Noteworthy is the
 * #push_back() function which actually rotates the buffer's contents. The
 * zero-argument `push_back()` function makes a "new" element available via
 * #back().
 */
template <typename T, class Container>
class RingBufferBase {
public:
    template <typename Tvalue, typename Tparent>
    class BaseIterator;
    typedef BaseIterator<const T, const RingBufferBase&> const_iterator;
    typedef BaseIterator<T, RingBufferBase&> iterator;

    void push_back() {
        add_entry() = std::move(T());
    }
    void push_back(const T& value) {
        add_entry() = value;
    }
    template <class... Args>
    void emplace_back(Args&&... args) {
        add_entry() = std::move(T(args...));
    }

    // Front/back
    [[nodiscard]] const T& front() const {
        return at(0);
    }
    [[nodiscard]] const T& back() const {
        return at(size() - 1);
    }
    T& front() {
        return at(0);
    }
    T& back() {
        return at(size() - 1);
    }

    T& operator[](size_t ix) {
        return at(ix);
    }
    const T& operator[](size_t ix) const {
        return at(ix);
    }

    [[nodiscard]] size_t size() const {
        return array.size();
    }

    void reset() {
        for (auto& e : array) {
            e = T();
        }
    }

    iterator begin() {
        return iterator(*this, 0);
    }
    iterator end() {
        return iterator(*this, size());
    }
    [[nodiscard]] const_iterator begin() const {
        return const_iterator(*this, 0);
    }
    [[nodiscard]] const_iterator end() const {
        return const_iterator(*this, size());
    }

    template <typename Tvalue, typename Tparent>
    class BaseIterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_const<Tvalue>;
        using pointer = Tvalue*;
        using reference = Tvalue&;
        using iterator_category = std::forward_iterator_tag;

        BaseIterator(Tparent& parent_, size_t pos_)
            : pos(pos_), parent(parent_) {
        }
        BaseIterator& operator++() {
            ++pos;
            return *this;
        }
        bool operator!=(const BaseIterator& other) const {
            return pos != other.pos;
        }
        Tvalue& operator*() {
            return parent[pos];
        }
        Tvalue* operator->() {
            return &(operator*());
        }

    private:
        size_t pos;
        Tparent& parent;
    };

protected:
    T& add_entry() {
        size_t last = first;
        first = (first + 1) % array.size();
        return array[last];
    }

    size_t first = 0;
    Container array;

private:
    [[nodiscard]] const T& at(size_t index) const {
        return array[(index + first) % size()];
    }
    T& at(size_t index) {
        return array[(index + first) % size()];
    }
};

/**
 * Ringbuffer backed by a std::array.
 * @param value_type value type to use
 * @param capacity the capacity/size of this ringbuffer
 */
template <typename T, size_t N>
class RingBuffer : public RingBufferBase<T, std::array<T, N>> {
public:
    using Parent = RingBufferBase<T, std::array<T, N>>;
    RingBuffer() {
        Parent::reset();
    }
};

/**
 * Ringbuffer backed by a std::vector. The capacity can be set at runtime using
 * the #reset() function.
 */
template <typename T>
class RingBufferVector : public RingBufferBase<T, std::vector<T>> {
public:
    using Parent = RingBufferBase<T, std::vector<T>>;

    RingBufferVector(size_t capacity = 0) {
        reset(capacity);
    }

    void reset(size_t capacity) {
        Parent::array.clear();
        Parent::array.resize(capacity);
    }
};

} // namespace
