/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once

#include <folly/Portability.h>
#include <gsl/gsl-lite.hpp>

#if FOLLY_X64
#include "scan_sse42.h"
#endif // FOLLY_X64

#if FOLLY_AARCH64
#include "scan_neon.h"
#endif // FOLLY_AARCH64

namespace cb::simd {

/**
 * Reads 16 bytes of data, then matches all of the characters passed in as
 * template parameters and returns the number of bytes until the first match
 * (or 16 if none of the characters was seen in the input).
 *
 * @tparam Chars The characters to match.
 * @param data The 16 byte input.
 * @return Number of characters until the first match
 */
template <char... Chars>
inline int scan_any_of_128bit(gsl::span<const unsigned char> data) {
    static_assert(sizeof...(Chars) != 0);
    auto bytes = detail::load_128bit(data);
    auto rv = detail::eq_any_of_128bit<Chars...>(bytes);
    return detail::scan_matches(rv);
}

} // namespace cb::simd
