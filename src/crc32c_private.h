/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2015 Couchbase, Inc
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
