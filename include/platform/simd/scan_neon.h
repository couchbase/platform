/*
 *     Copyright 2023-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/Portability.h>
#include <gsl/gsl-lite.hpp>

// Guarded here so clang-tidy does not complain if it is not running on ARM host
#if FOLLY_AARCH64

#include <arm_acle.h>
#include <arm_neon.h>

namespace cb::simd::detail {

using simd_vector_t = uint8x16_t;

/**
 * Load some data into a vector.
 */
inline simd_vector_t load_128bit(gsl::span<const unsigned char> data) {
#if CB_DEVELOPMENT_ASSERTS
    Expects(data.size() >= 16);
#endif
    // Load 16 bytes into an SIMD register.
    return vld1q_u8(data.data());
}

/**
 * Set all bits of any matching elements to 1.
 */
template <char... Chars>
inline simd_vector_t eq_any_of_128bit(simd_vector_t bytes) {
    // Set all bits of any matching elements to 1.
    return (... | vceqq_u8(bytes, vdupq_n_u8(Chars)));
}

/**
 * Count the number of elements until the first match in the result of a logical
 * operation (after eq_any_of_128bit).
 * @return the number of elements until the match or the number of elements in
 * the vector if everything matches.
 */
inline int scan_matches(simd_vector_t rv) {
    // (16 x elements) ... 00000000 11111111 00000000 11111111
    // <<shift right by 4 and truncate>> gives
    // (16 x elements) ... 00000000 00000000 00001111 00001111
    auto mask = vget_lane_u64(
            vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(rv), 4)), 0);
    // Count the number of _trailing_ zero bits to the first group of 4 '1's.
    // Then divide by 4 to get the number of groups until that group (number
    // of non-matching bytes).
    // Note on UB of __builtin_ctz: GCC/clang will compile this to a rbit+clz.
    // which is well-defined to return 64 if the register contains only zeros.
    // Unfortunately, older versions of GCC do not have the rbit intrinsic.
    return __builtin_ctzll(mask) >> 2;
}

} // namespace cb::simd::detail

#endif // FOLLY_AARCH64
