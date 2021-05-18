/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/**
 * Variadic function wrapper around std::bitset. The general intent is that if
 * you had:
 *  enum states {a, b, c, d};
 *
 * You can create bitset to represent a set of states:
 *
 *  cb::bitset<4, states> permittedStates(a, c);
 *  std::cout << permittedStates << "\n"; // 0101
 *
 * Or if you had:
 *   enum states {a = 1, b, c, d};
 *   struct states_mapper {
 *      size_t map(states in) {
 *          return in - 1;
 *      }
 *   }
 *   cb::bitset<4, states, states_mapper> permittedStates(a, d);
 *   std::cout << permittedStates << "\n"; // 1001
 */

#pragma once

#include <bitset>

namespace cb {
template <class Type>
struct default_bitset_mapper {
    [[nodiscard]] constexpr size_t map(Type t) const {
        return size_t(t);
    }
};

template <std::size_t N, class Type, class Map = default_bitset_mapper<Type>>
class bitset {
public:
    /// Default construct an empty bitset
    bitset() {
    }

    /// Construct bitset setting the specified values
    template <class... Targs>
    bitset(Type t, Targs... Fargs) {
        init(t, Fargs...);
    }

    /// map and set the input t
    void set(Type t) {
        bits.set(Map().map(t));
    }

    /// map and reset the input t
    void reset(Type t) {
        bits.reset(Map().map(t));
    }

    /// map and test the input t
    [[nodiscard]] bool test(Type t) const {
        return bits.test(Map().map(t));
    }

private:
    template <class... Targs>
    void init(Type t, Targs... Fargs) {
        init(t);
        init(Fargs...);
    }

    void init(Type t) {
        this->set(t);
    }

    std::bitset<N> bits;
};

} // end namespace cb
