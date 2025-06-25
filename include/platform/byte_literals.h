/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <cstddef>

/**
 * The type used for byte literals.
 * Note: This is an unsigned type, so it can't be used to represent negative
 * values.
 */
using ByteLiteralType = std::size_t;

/**
 * Helper to create a std::size_t from a number of kibibytes.
 */
constexpr ByteLiteralType operator""_KiB(unsigned long long kibibytes) {
    return kibibytes * 1024;
}

/**
 * Helper to create a std::size_t from a number of kibibytes.
 */
constexpr ByteLiteralType operator""_KiB(long double kibibytes) {
    return static_cast<ByteLiteralType>(kibibytes * 1024);
}

/**
 * Helper to create a std::size_t from a number of mebibytes.
 */
constexpr ByteLiteralType operator""_MiB(unsigned long long mebibytes) {
    return mebibytes * 1024_KiB;
}

/**
 * Helper to create a std::size_t from a number of mebibytes.
 */
constexpr ByteLiteralType operator""_MiB(long double mebibytes) {
    return static_cast<ByteLiteralType>(mebibytes * 1024_KiB);
}

/**
 * Helper to create a std::size_t from a number of gibibytes.
 */
constexpr ByteLiteralType operator""_GiB(unsigned long long gibibytes) {
    return gibibytes * 1024_MiB;
}

/**
 * Helper to create a std::size_t from a number of gibibytes.
 */
constexpr ByteLiteralType operator""_GiB(long double gibibytes) {
    return static_cast<ByteLiteralType>(gibibytes * 1024_MiB);
}

/**
 * Helper to create a std::size_t from a number of tibibytes.
 */
constexpr ByteLiteralType operator""_TiB(unsigned long long tibibytes) {
    return tibibytes * 1024_GiB;
}

/**
 * Helper to create a std::size_t from a number of tibibytes.
 */
constexpr ByteLiteralType operator""_TiB(long double tibibytes) {
    return static_cast<ByteLiteralType>(tibibytes * 1024_GiB);
}
