/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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
    constexpr size_t map(Type t) const {
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
    bool test(Type t) const {
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
