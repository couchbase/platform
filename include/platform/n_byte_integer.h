/*
 *     Copyright 2018-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <folly/Portability.h>

#include <algorithm>
#include <array>
#include <cstdint>

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

    [[nodiscard]] UnsignedNByteInteger byteSwap() const {
        auto ret = UnsignedNByteInteger<N>();
        std::reverse_copy(counter.begin(), counter.end(), ret.counter.begin());
        return ret;
    }

    [[nodiscard]] UnsignedNByteInteger hton() const {
        if (folly::kIsLittleEndian) {
            return byteSwap();
        }

        return *this;
    }

    [[nodiscard]] UnsignedNByteInteger ntoh() const {
        return hton();
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
    [[nodiscard]] uint64_t load() const {
        uint64_t value = 0;
        if (folly::kIsLittleEndian) {
            std::copy_n(counter.begin(), N, reinterpret_cast<uint8_t*>(&value));
        } else {
            std::copy_n(counter.begin(),
                        N,
                        ((8 - N) + reinterpret_cast<uint8_t*>(&value)));
        }
        return value;
    }

    void store(uint64_t value) {
        if (folly::kIsLittleEndian) {
            std::copy_n(reinterpret_cast<uint8_t*>(&value), N, counter.begin());
        } else {
            std::copy_n((reinterpret_cast<uint8_t*>(&value) + (8 - N)),
                        N,
                        counter.begin());
        }
    }

    /// The current value of the n-byte integer
    std::array<uint8_t, N> counter;
};

using uint48_t = UnsignedNByteInteger<6>;

inline uint64_t format_as(uint48_t val) {
    return val;
}

} // namespace cb
