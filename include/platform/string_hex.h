/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#pragma once


#include "sized_buffer.h"
#include <cstdint>

namespace cb {
/**
 * Get the value for a string of hex characters
 *
 * @param buffer the input buffer
 * @return the value of the string
 * @throws std::invalid_argument for an invalid character in the string
 *         std::overflow_error if the input string won't fit in uint64_t
 */
uint64_t from_hex(std::string_view buffer);

std::string to_hex(uint8_t val);

std::string to_hex(uint16_t val);

std::string to_hex(uint32_t val);

std::string to_hex(uint64_t val);

std::string to_hex(const_byte_buffer buffer);

/**
 * Encode a sequence of bytes in hex: (ex: {0xde, 0xad, 0xca, 0xfe} would
 * return "deadcafe"
 *
 * @param buffer Input data to dump
 * @return the hex string
 */
std::string hex_encode(const_byte_buffer buffer);

inline std::string hex_encode(std::string_view buffer) {
    return hex_encode(
            {reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size()});
}

} // namespace cb
