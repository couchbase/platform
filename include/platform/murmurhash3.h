//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#pragma once

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

#include <cstddef>
#include <cstdint>

//-----------------------------------------------------------------------------

void MurmurHash3_x86_32(const void* key,
                        size_t len,
                        uint32_t seed,
                        uint32_t* out);

/**
 * The following 2 functions have been altered to just return the
 * 64 most significant bits.
 */
void MurmurHash3_x86_128(const void* key,
                         size_t len,
                         uint32_t seed,
                         uint64_t* out);

void MurmurHash3_x64_128(const void* key,
                         size_t len,
                         uint32_t seed,
                         uint64_t* out);

//-----------------------------------------------------------------------------
