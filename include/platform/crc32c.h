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
// Generate a CRC-32C (Castagnolia)
// CRC polynomial of 0x1EDC6F41
//
// When available a hardware assisted function is used for increased
// performance.
//
#pragma once
#include <cstddef>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#define CB_CRC32_HW_SUPPORTED 1
#endif

uint32_t crc32c(const uint8_t* buf, size_t len, uint32_t crc_in);

// The following methods are used by unit testing to force the calculation
// of the checksum by using a given implementation.
uint32_t crc32c_sw(const uint8_t* buf, size_t len, uint32_t crc_in);
#ifdef CB_CRC32_HW_SUPPORTED
uint32_t crc32c_hw(const uint8_t* buf, size_t len, uint32_t crc_in);
uint32_t crc32c_hw_1way(const uint8_t* buf, size_t len, uint32_t crc_in);
#endif
