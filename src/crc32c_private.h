/*
 *     Copyright 2015-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

//
// crc32c_private - constants/functions etc...
// required by software and hardware implementations
//

#pragma once
#include <cstdint>

const uintptr_t ALIGN64_MASK = sizeof(uint64_t)-1;
const int LONG_BLOCK = 8192;
const int SHORT_BLOCK = 256;
const int SHIFT_TABLE_X = 4, SHIFT_TABLE_Y = 256;
extern uint32_t crc32c_long[SHIFT_TABLE_X][SHIFT_TABLE_Y];
extern uint32_t crc32c_short[SHIFT_TABLE_X][SHIFT_TABLE_Y];

/* Apply the zeros operator table to crc. */
inline uint32_t crc32c_shift(uint32_t zeros[SHIFT_TABLE_X][SHIFT_TABLE_Y],
                      uint32_t crc) {
    return zeros[0][crc & 0xff] ^ zeros[1][(crc >> 8) & 0xff] ^
           zeros[2][(crc >> 16) & 0xff] ^ zeros[3][crc >> 24];
}
