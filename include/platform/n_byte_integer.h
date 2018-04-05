/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2018 Couchbase, Inc.
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

#include "config.h"

#include <algorithm>
#include <array>

namespace cb {

/**
 * UnsignedNByteInteger provides a non-atomic integral type for 3/5/6 or 7 byte
 * integers.
 *
 * All operations occur as unsigned 64-bit operations with the result stored
 * back to the smaller type with the most-significant bytes removed.
 *
 */
template <size_t N>
class UnsignedNByteInteger {
    static_assert(
            (N < sizeof(uint64_t)),
            "UnsignedNByteInteger: size should be less than sizeof(uint64_t)");
    static_assert((N != sizeof(uint32_t)),
                  "UnsignedNByteInteger: use uint32_t");
    static_assert((N != sizeof(uint16_t)),
                  "UnsignedNByteInteger: use uint16_t");
    static_assert((N != sizeof(uint8_t)), "UnsignedNByteInteger: use uint8_t");

public:
    /// Initialise to zero
    UnsignedNByteInteger() : counter{} {
    }

    /// Initialise to n
    UnsignedNByteInteger(uint64_t n) : counter{} {
        store(n);
    }

    operator uint64_t() const {
        return load();
    }

    UnsignedNByteInteger<N>& operator+=(uint64_t in) {
        fetch_add(in);
        return *this;
    }

    UnsignedNByteInteger<N>& operator-=(uint64_t in) {
        fetch_sub(in);
        return *this;
    }

    uint64_t operator++() {
        return fetch_add(1) + 1;
    }

    uint64_t operator++(int) {
        return fetch_add(1);
    }

    uint64_t operator--() {
        return fetch_sub(1) - 1;
    }

    uint64_t operator--(int) {
        return fetch_sub(1);
    }

private:
    uint64_t fetch_add(uint64_t n) {
        uint64_t value = load();
        store(value + n);
        return value;
    }

    uint64_t fetch_sub(uint64_t n) {
        uint64_t value = load();
        store(value - n);
        return value;
    }

    /// @return the current value as a uint64_t
    uint64_t load() const {
        uint64_t value = 0;
        std::copy_n(counter.begin(), N, reinterpret_cast<uint8_t*>(&value));
        return value;
    }

    void store(uint64_t value) {
        std::copy_n(reinterpret_cast<uint8_t*>(&value), N, counter.begin());
    }

    /// The current value of the n-byte integer
    std::array<uint8_t, N> counter;
};

using uint48_t = UnsignedNByteInteger<6>;

} // namespace cb
