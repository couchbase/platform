/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2017 Couchbase, Inc.
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
#pragma once

#include <platform/visibility.h>

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
PLATFORM_PUBLIC_API
uint64_t from_hex(std::string_view buffer);

PLATFORM_PUBLIC_API
std::string to_hex(uint8_t val);

PLATFORM_PUBLIC_API
std::string to_hex(uint16_t val);

PLATFORM_PUBLIC_API
std::string to_hex(uint32_t val);

PLATFORM_PUBLIC_API
std::string to_hex(uint64_t val);

PLATFORM_PUBLIC_API
std::string to_hex(const_byte_buffer buffer);

/**
 * Encode a sequence of bytes in hex: (ex: {0xde, 0xad, 0xca, 0xfe} would
 * return "deadcafe"
 *
 * @param buffer Input data to dump
 * @return the hex string
 */
PLATFORM_PUBLIC_API
std::string hex_encode(const_byte_buffer buffer);

inline std::string hex_encode(std::string_view buffer) {
    return hex_encode(
            {reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size()});
}

} // namespace cb
