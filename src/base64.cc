/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

/*
 * Function to base64 encode and decode text as described in RFC 4648
 */

#include <platform/base64.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

/// An array of the legal characters used for direct lookup
static const uint8_t code[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * A method to map the code back to the value
 *
 * @param code the code to map
 * @return the byte value for the code character
 */
static uint32_t code2val(const uint8_t code) {
    if (code >= 'A' && code <= 'Z') {
        return code - 'A';
    }
    if (code >= 'a' && code <= 'z') {
        return code - 'a' + uint8_t(26);
    }
    if (code >= '0' && code <= '9') {
        return code - '0' + uint8_t(52);
    }
    if (code == '+') {
        return uint8_t(62);
    }
    if (code == '/') {
        return uint8_t(63);
    }
    throw std::invalid_argument("cb::base64::code2val Invalid input character");
}

/**
 * Encode up to 3 characters to 4 output character.
 *
 * @param s pointer to the input stream
 * @param d pointer to the output stream
 * @param num the number of characters from s to encode
 */
static void encode_rest(const uint8_t* s, std::string& result, size_t num) {
    uint32_t val = 0;

    switch (num) {
    case 2:
        val = (uint32_t)((*s << 16) | (*(s + 1) << 8));
        break;
    case 1:
        val = (uint32_t)((*s << 16));
        break;
    default:
        throw std::invalid_argument("base64::encode_rest num may be 1 or 2");
    }

    result.push_back((char)code[(val >> 18) & 63]);
    result.push_back((char)code[(val >> 12) & 63]);
    if (num == 2) {
        result.push_back((char)code[(val >> 6) & 63]);
    } else {
        result.push_back('=');
    }
    result.push_back('=');
}

/**
 * Encode 3 bytes to 4 output character.
 *
 * @param s pointer to the input stream
 * @param d pointer to the output stream
 */
static void encode_triplet(const uint8_t* s, std::string& str) {
    auto val = (uint32_t)((*s << 16) | (*(s + 1) << 8) | (*(s + 2)));
    str.push_back((char)code[(val >> 18) & 63]);
    str.push_back((char)code[(val >> 12) & 63]);
    str.push_back((char)code[(val >> 6) & 63]);
    str.push_back((char)code[val & 63]);
}

/**
 * decode 4 input characters to up to two output bytes
 *
 * @param s source string
 * @param d destination
 * @return the number of characters inserted
 */
static int decode_quad(const uint8_t* s, std::string& d) {
    uint32_t value = code2val(s[0]) << 18;
    value |= code2val(s[1]) << 12;

    int ret = 3;

    if (s[2] == '=') {
        ret = 1;
    } else {
        value |= code2val(s[2]) << 6;
        if (s[3] == '=') {
            ret = 2;
        } else {
            value |= code2val(s[3]);
        }
    }

    d.push_back(uint8_t(value >> 16));
    if (ret > 1) {
        d.push_back(uint8_t(value >> 8));
        if (ret > 2) {
            d.push_back(uint8_t(value));
        }
    }

    return ret;
}

namespace cb::base64 {
std::string encode(std::string_view view, bool prettyprint) {
    // base64 encoding encodes up to 3 input characters to 4 output
    // characters in the alphabet above.
    auto triplets = view.size() / 3;
    auto rest = view.size() % 3;
    auto chunks = triplets;
    if (rest != 0) {
        ++chunks;
    }

    std::string result;
    if (prettyprint) {
        // In pretty-print mode we insert a newline after adding
        // 16 chunks (four characters).
        result.reserve(chunks * 4 + chunks / 16);
    } else {
        result.reserve(chunks * 4);
    }

    const auto* in = reinterpret_cast<const uint8_t*>(view.data());

    chunks = 0;
    for (size_t ii = 0; ii < triplets; ++ii) {
        encode_triplet(in, result);
        in += 3;

        if (prettyprint && (++chunks % 16) == 0) {
            result.push_back('\n');
        }
    }

    if (rest > 0) {
        encode_rest(in, result, rest);
    }

    if (prettyprint && result.back() != '\n') {
        result.push_back('\n');
    }

    return result;
}

std::string decode(std::string_view blob) {
    std::string destination;

    if (blob.empty()) {
        return {};
    }

    // To reduce the number of reallocations, start by reserving an
    // output buffer of 75% of the input size (and add 3 to avoid dealing
    // with zero)
    size_t estimate = blob.size() * 0.75;
    destination.reserve(estimate + 3);

    const auto* in = reinterpret_cast<const uint8_t*>(blob.data());
    size_t offset = 0;
    while (offset < blob.size()) {
        if (std::isspace((int)*in)) {
            ++offset;
            ++in;
            continue;
        }

        // We need at least 4 bytes
        if ((offset + 4) > blob.size()) {
            throw std::invalid_argument("cb::base64::decode invalid input");
        }

        decode_quad(in, destination);
        in += 4;
        offset += 4;
    }

    return destination;
}

} // namespace cb::base64

namespace cb::base64url {
std::string encode(std::string_view source) {
    auto encoded = base64::encode(source, false);
    // Replace the all use of '+' with '-' and '/' with '_'
    for (auto& c: encoded) {
        if (c == '+') {
            c = '-';
        } else if (c == '/') {
            c = '_';
        }
    }

    // The padding bytes should be stripped off as '=' would be encoded with
    // % in a URL
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }

    return encoded;
}
std::string decode(std::string_view source) {
    // convert back to a base64 encoded string and decode that
    std::string string{source};
    for (auto& c : string) {
        if (c == '-') {
            c = '+';
        } else if (c == '_') {
            c = '/';
        }
    }

    // decode expects the input string to contain padding bytes
    while (string.size() % 4) {
        string.push_back('=');
    }

    return base64::decode(string);
}
}
