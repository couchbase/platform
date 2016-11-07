/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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
    const T& front() const {
        return at(0);
    }
    const T& back() const {
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

    size_t size() const {
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
    const_iterator begin() const {
        return const_iterator(*this, 0);
    }
    const_iterator end() const {
        return const_iterator(*this, size());
    }

    template <typename Tvalue, typename Tparent>
    class BaseIterator {
    public:
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
    const T& at(size_t index) const {
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
