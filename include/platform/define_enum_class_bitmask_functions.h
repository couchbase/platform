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

/**
 * Define operators (|, |=, &, &= and ~) and a function to check if a bit
 * is set in the enum (bitmask)
 *
 * @param T The name of the enum to define the operators for
 */
#define DEFINE_ENUM_CLASS_BITMASK_FUNCTIONS(T)                                 \
    constexpr T operator|(T a, T b) {                                          \
        return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b)); \
    }                                                                          \
    constexpr T& operator|=(T& a, T b) {                                       \
        a = a | b;                                                             \
        return a;                                                              \
    }                                                                          \
    constexpr T operator&(T a, T b) {                                          \
        return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b)); \
    }                                                                          \
    constexpr T& operator&=(T& a, T b) {                                       \
        a = a & b;                                                             \
        return a;                                                              \
    }                                                                          \
    constexpr T operator~(T a) {                                               \
        return T(~std::underlying_type_t<T>(a));                               \
    }                                                                          \
    constexpr bool isFlagSet(T mask, T flag) {                                 \
        return (mask & flag) == flag;                                          \
    }
