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


#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <iosfwd>

namespace cb {

/**
 * Struct representing a buffer of some known size. This is used to
 * refer to some existing region of memory which is owned elsewhere - i.e.
 * this object does not have ownership semantics.
 * A user should not free() the buf member themselves!
 *
 * @todo more member functions can be made constexpr in C++14
 *       without implicit const / with generalised constexpr
 */
template <typename T>
struct sized_buffer {
    using value_type = T;
    using base_type = typename std::remove_const<T>::type;

    // Type of instantiation
    using buffer_type = sized_buffer<value_type>;

    // Const variant of instantiation (may be same as buffer_type)
    using cbuffer_type = sized_buffer<const base_type>;

    // Non-const variant of instantiation (may be same as buffer_type)
    using ncbuffer_type = sized_buffer<base_type>;

    using pointer = value_type*;
    using const_pointer = const base_type*;
    using reference = value_type&;
    using const_reference = const base_type&;

    using iterator = pointer;
    using const_iterator = const_pointer;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    static const size_type npos = size_type(-1);

    constexpr sized_buffer() : sized_buffer(nullptr, 0) {
    }

    constexpr sized_buffer(pointer buf_, size_type len_) : buf(buf_), len(len_) {
    }

    /**
     * Constructor from const reference to a string
     * (cannot be instantiated for non-const T)
     */
    sized_buffer(const std::basic_string<base_type>& str)
        : sized_buffer(str.data(), str.size()) {
    }

    /**
     * Constructor from non-const reference to a string
     */
    sized_buffer(std::basic_string<base_type>& str)
        : sized_buffer(&str[0], str.size()) {
    }

    /**
     * Constructor from const reference to a vector
     * (cannot be instantiated for non-const T)
     */
    sized_buffer(const std::vector<base_type>& vec)
        : sized_buffer(vec.data(), vec.size()) {
    }

    /**
     * Constructor from non-const reference to a vector
     */
    sized_buffer(std::vector<base_type>& vec)
        : sized_buffer(vec.data(), vec.size()) {
    }

    /**
     * Copy constructor from sized_buffer<> types
     */
    template <typename B>
    sized_buffer(const sized_buffer<B>& other)
        : buf(const_cast<T*>(other.data())), len(other.size()) {
    }

    // Iterators

    iterator begin() {
        return buf;
    }

    [[nodiscard]] constexpr const_iterator begin() const {
        return buf;
    }

    [[nodiscard]] constexpr const_iterator cbegin() const {
        return buf;
    }

    iterator end() {
        return buf + size();
    }

    [[nodiscard]] constexpr const_iterator end() const {
        return buf + size();
    }

    [[nodiscard]] constexpr const_iterator cend() const {
        return buf + size();
    }

    // Element access

    reference operator[](size_type pos) {
        return buf[pos];
    }

    constexpr const_reference operator[](size_type pos) const {
        return buf[pos];
    }

    reference at(size_type pos) {
        if (pos >= size()) {
            throw std::out_of_range(
                    std::string("cb::sized_buffer<") + typeid(T).name() +
                    ">::at: 'pos' out of range (" + std::to_string(pos) +
                    " > " + std::to_string(size()) + ")");
        }
        return buf[pos];
    }

    [[nodiscard]] const_reference at(size_type pos) const {
        if (pos >= size()) {
            throw std::out_of_range(
                    std::string("cb::sized_buffer<") + typeid(T).name() +
                    ">::at: 'pos' out of range (" + std::to_string(pos) +
                    " > " + std::to_string(size()) + ")");
        }
        return buf[pos];
    }

    reference front() {
        return buf[0];
    }

    [[nodiscard]] constexpr const_reference front() const {
        return buf[0];
    }

    reference back() {
        return buf[size() - 1];
    }

    [[nodiscard]] constexpr const_reference back() const {
        return buf[size() - 1];
    }

    pointer data() {
        return buf;
    }

    [[nodiscard]] constexpr const_pointer data() const {
        return buf;
    }

    // Capacity

    [[nodiscard]] constexpr size_type size() const {
        return len;
    }

    [[nodiscard]] constexpr bool empty() const {
        return size() == 0;
    }

    // Operations

    /**
     * Returns a buffer of the substring [pos, pos + rcount),
     * where rcount is the smaller of count and size() - pos.
     */
    sized_buffer<value_type> substr(size_type pos = 0, size_type count = npos) {
        if (pos > size()) {
            throw std::out_of_range(
                    std::string("cb::sized_buffer<") + typeid(T).name() +
                    ">::substr: 'pos' out of range (" + std::to_string(pos) +
                    " > " + std::to_string(size()) + ")");
        }

        const size_type rcount = std::min(count, size() - pos);
        return buffer_type(data() + pos, rcount);
    }

    /**
     * Compares two character sequences.
     *
     * The length rlen of the sequences to compare is the smaller of size() and
     * v.size(). The function compares the two views by calling
     * traits::compare(data(), v.data(), rlen). If the result is nonzero then
     * it is returned. Otherwise, returns a value as indicated:
     *
     *  - size() < v.size(): -1
     *  - size() == v.size(): 0
     *  - size() > v.size(): 1
     */
    [[nodiscard]] int compare(buffer_type v) const {
        const size_type rlen = std::min(size(), v.size());
        const int cmp =
                std::char_traits<base_type>::compare(data(), v.data(), rlen);

        if (cmp != 0) {
            return cmp;
        } else if (size() < v.size()) {
            return -1;
        } else if (size() > v.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    /**
     * Finds the first occurrence of v in this buffer, starting at position pos.
     *
     * @return Position of the first character of the found substring,
     *         or npos if no such substring is found.
     */
    [[nodiscard]] size_type find(cbuffer_type v, size_type pos = 0) const {
        if (pos > size()) {
            return npos;
        }

        auto ptr = std::search(begin() + pos, end(), v.begin(), v.end());
        if (ptr == end()) {
            return npos;
        } else {
            return ptr - begin();
        }
    }

    /**
     * Finds the first occurrence of any of the characters of v in this buffer,
     * starting at position pos.
     *
     * @return Position of the first occurrence of any character of the
     *         substring, or npos if no such character is found.
     */
    [[nodiscard]] size_type find_first_of(cbuffer_type v,
                                          size_type pos = 0) const {
        if (pos > size()) {
            return npos;
        }

        auto ptr = std::find_first_of(begin() + pos, end(), v.begin(), v.end());
        if (ptr == end()) {
            return npos;
        } else {
            return ptr - begin();
        }
    }

private:
    pointer buf;
    size_type len;
};

template <class T>
const size_t sized_buffer<T>::npos;

template <class CharT>
bool operator==(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) == 0;
}

template <class CharT>
bool operator!=(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) != 0;
}

template <class CharT>
bool operator<(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) < 0;
}

template <class CharT>
bool operator>(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) > 0;
}

template <class CharT>
bool operator<=(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) <= 0;
}

template <class CharT>
bool operator>=(sized_buffer<CharT> lhs, sized_buffer<CharT> rhs) {
    return lhs.compare(rhs) >= 0;
}

namespace {
template <typename CharT>
size_t buffer_hash(sized_buffer<CharT> base) {
    // Probably only needs trivially copyable
    static_assert(std::is_pod<CharT>::value,
                  "cb::buffer_hash: Template param CharT must be a POD");

    // Perform the hash over the raw bytes of the CharT array
    const size_t size = base.size() * sizeof(CharT);
    const auto* bytes = reinterpret_cast<const uint8_t*>(base.data());

    size_t rv = 5381;
    for (size_t ii = 0; ii < size; ii++) {
        rv = ((rv << 5) + rv) ^ size_t(bytes[ii]);
    }
    return rv;
}
}

/**
 * The char_buffer is intended for strings you might want to modify
 */
using char_buffer = sized_buffer<char>;

std::string to_string(char_buffer cb);

/**
 * The byte_buffer is intended to use for a blob of bytes you might want
 * to modify
 */
using byte_buffer = sized_buffer<uint8_t>;

/**
 * The const_byte_buffer is intended for a blob of bytes you _don't_
 * want to modify.
 */
using const_byte_buffer = sized_buffer<const uint8_t>;

} // namespace cb

namespace std {
template <typename CharT>
struct hash<cb::sized_buffer<CharT>> {
    size_t operator()(cb::sized_buffer<CharT> s) const {
        return cb::buffer_hash(s);
    }
};
}
