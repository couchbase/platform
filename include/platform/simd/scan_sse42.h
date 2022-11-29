/*
 *     Copyright 2022-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/Portability.h>
#include <gsl/gsl-lite.hpp>

// Guarded here so clang-tidy does not complain if it is not running on x86 host
#if FOLLY_X64

#include <cstdint>
#include <cstring>

#if defined(_WIN32)
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
#include <smmintrin.h>
#endif

namespace cb::simd {

template <char... Chars>
inline int scan_any_of_128bit(gsl::span<const unsigned char> data) {
#if CB_DEVELOPMENT_ASSERTS
    Expects(data.size() >= 16);
#endif
    static_assert(sizeof...(Chars) != 0);
    uint64_t r;
    memcpy(&r, data.data(), sizeof(uint64_t));
    uint64_t r2;
    memcpy(&r2, data.data() + sizeof(uint64_t), sizeof(uint64_t));
    // Load the two 64-bit integers into an SSE register.
    auto bytes = _mm_set_epi64x(r2, r);
    // Set all bits of any matching elements to 1.
#if defined(_WIN32)
    // MSVC from VS 2017 struggles with the variadic expansion used for other
    // compilers. Just use a for loop - it wasn't unrolled in my testing, but
    // even with the additional cost of the conditional branch for the counter,
    // we should still see a net gain by using this vectorised procedure.
    constexpr char chars[]{Chars...};
    auto rv = _mm_cmpeq_epi8(bytes, _mm_set1_epi8(chars[0]));
    for (size_t i = 1; i < sizeof...(Chars); i++) {
        rv = _mm_or_si128(rv, _mm_cmpeq_epi8(bytes, _mm_set1_epi8(chars[i])));
    }
#else
    auto rv = (... | _mm_cmpeq_epi8(bytes, _mm_set1_epi8(Chars)));
#endif
    // Extract the highest order bit of each byte in the vector.
    // This will be 1 if the element was equal to any character and 0 otherwise.
    auto mask = _mm_movemask_epi8(rv);
    // Count the number of trailing zero bits (= number of elements which did
    // not match anything), but limit the count to 16 by OR-ing with a value
    // which has bit 17 set to 1.
    mask |= 0x10000;
#if defined(_WIN32)
    return _tzcnt_u32(mask);
#else
    return __builtin_ctz(mask);
#endif
}

} // namespace cb::simd

#endif // FOLLY_X64
