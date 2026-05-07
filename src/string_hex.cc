/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <platform/string_hex.h>

#include <charconv>
#include <cinttypes>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

uint64_t cb::from_hex(std::string_view buffer) {
    if (buffer.starts_with("0x")) {
        buffer = buffer.substr(2);
    }

    if (buffer.starts_with("-")) {
        throw std::invalid_argument(
                "cb::from_hex: negative values are not allowed");
    }

    uint64_t ret = 0;
    const auto [ptr, ec] = std::from_chars(
            buffer.data(), buffer.data() + buffer.size(), ret, 16);

    if (ec != std::errc()) {
        if (ec == std::errc::invalid_argument) {
            throw std::invalid_argument(
                    "cb::from_hex: character was not in hexadecimal range");
        }
        if (ec == std::errc::result_out_of_range) {
            throw std::overflow_error("cb::from_hex: value out of range");
        }
        throw std::system_error(std::make_error_code(ec),
                                "cb::from_hex: from_chars failed");
    }
    if (ptr != buffer.data() + buffer.size()) {
        throw std::invalid_argument(
                "cb::from_hex: not all characters were parsed");
    }

    return ret;
}

std::string cb::to_hex(uint8_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%02" PRIx8, val);
    return std::string{buf};
}

std::string cb::to_hex(uint16_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%04" PRIx16, val);
    return std::string{buf};
}

std::string cb::to_hex(uint32_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08" PRIx32, val);
    return std::string{buf};
}

std::string cb::to_hex(uint64_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016" PRIx64, val);
    return std::string{buf};
}

std::string cb::to_hex(cb::const_byte_buffer buffer) {
    if (buffer.empty()) {
        return "";
    }
    std::stringstream ss;
    for (const auto& c : buffer) {
        ss << "0x" << std::hex << std::setfill('0') << std::setw(2)
           << uint32_t(c) << " ";
    }
    auto ret = ss.str();
    ret.resize(ret.size() - 1);
    return ret;
}

std::string cb::hex_encode(cb::const_byte_buffer buffer) {
    if (buffer.empty()) {
        return "";
    }
    std::stringstream ss;
    for (const auto& c : buffer) {
        ss << std::hex << std::setfill('0') << std::setw(2) << uint32_t(c);
    }
    auto ret = ss.str();
    return ret;
}
