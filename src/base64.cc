/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
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

/*
 * Function to base64 encode and decode text as described in RFC 4648
 *
 * @author Trond Norbye
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <platform/base64.h>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * An array of the legal characters used for direct lookup
 */
static const uint8_t code[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * A method to map the code back to the value
 *
 * @param code the code to map
 * @return the byte value for the code character
 */
static const uint32_t code2val(const uint8_t code) {
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
    throw std::invalid_argument("Couchbase::base64::code2val Invalid "
                                    "input character");
}

/**
 * Encode up to 3 characters to 4 output character.
 *
 * @param s pointer to the input stream
 * @param d pointer to the output stream
 * @param num the number of characters from s to encode
 */
static void encode_rest(const uint8_t* s, uint8_t* d, size_t num) {
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

    d[3] = '=';

    if (num == 2) {
        d[2] = code[(val >> 6) & 63];
    } else {
        d[2] = '=';
    }

    d[1] = code[(val >> 12) & 63];
    d[0] = code[(val >> 18) & 63];
}

/**
 * Encode 3 bytes to 4 output character.
 *
 * @param s pointer to the input stream
 * @param d pointer to the output stream
 */
static void encode_triplet(const uint8_t* s, uint8_t* d) {
    uint32_t val = (uint32_t)((*s << 16) | (*(s + 1) << 8) | (*(s + 2)));
    d[3] = code[val & 63];
    d[2] = code[(val >> 6) & 63];
    d[1] = code[(val >> 12) & 63];
    d[0] = code[(val >> 18) & 63];
}

std::string Couchbase::Base64::encode(const std::string& source) {
    // base64 encoding encodes up to 3 input characters to 4 output
    // characters in the alphabet above.
    auto triplets = source.length() / 3;
    auto rest = source.length() % 3;
    auto chunks = triplets;
    if (rest != 0) {
        ++chunks;
    }

    std::vector<char> buffer(chunks * 4);

    const uint8_t* in = reinterpret_cast<const uint8_t*>(source.data());
    uint8_t* out = reinterpret_cast<uint8_t*>(buffer.data());

    for (size_t ii = 0; ii < triplets; ++ii) {
        encode_triplet(in, out);
        in += 3;
        out += 4;
    }

    if (rest > 0) {
        encode_rest(in, out, rest);
    }

    return std::string(buffer.data(), buffer.size());
}

/**
 * decode 4 input characters to up to two output bytes
 *
 * @param s source string
 * @param d destination
 * @return the number of characters inserted
 */
static int decode_quad(const uint8_t* s, uint8_t* d) {
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

    d[0] = uint8_t(value >> 16);
    d[1] = uint8_t(value >> 8);
    d[2] = uint8_t(value);

    return ret;
}

std::string Couchbase::Base64::decode(const std::string& source) {

    if (source.length() == 0) {
        return "";
    }

    if (source.length() % 4 != 0) {
        throw std::invalid_argument("Couchbase::Base64::decode invalid "
                                        "input length");
    }

    std::vector<uint8_t> destination;
    destination.resize(source.size());

    const uint8_t* in = reinterpret_cast<const uint8_t*>(source.data());
    uint8_t* out = destination.data();
    size_t outlen = 0;

    size_t quads = source.length() / 4;
    for (std::string::size_type ii = 0; ii < quads; ++ii) {
        int num = decode_quad(in, out);
        in += 4;
        out += num;
        outlen += num;
    }

    return std::string(reinterpret_cast<char*>(destination.data()), outlen);
}

