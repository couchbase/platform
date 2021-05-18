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

#include <cinttypes>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

static inline uint8_t from_hex_digit(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('A' <= c && c <= 'F') {
        return c + 10 - 'A';
    } else if ('a' <= c && c <= 'f') {
        return c + 10 - 'a';
    }
    throw std::invalid_argument(
            "cb::from_hex_digit: character was not in hexadecimal range");
}

uint64_t cb::from_hex(std::string_view buffer) {
    uint64_t ret = 0;

    if (buffer.find("0x") == 0) {
        // trim off the 0x
        buffer = buffer.substr(2);
    }

    if (buffer.size() > 16) {
        throw std::overflow_error("cb::from_hex: input string too long: " +
                                  std::to_string(buffer.size()));
    }

    for (char digit : buffer) {
        ret = (ret << 4) | from_hex_digit(digit);
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
