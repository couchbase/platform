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

#include <gtest/gtest.h>
#include <platform/string.h>

TEST(Hex, InputStringTooLong) {
    std::stringstream ss;
    ss << std::hex << uint64_t(-1);
    EXPECT_EQ(-1ULL, cb::from_hex(ss.str()));
    // Make sure it won't fit in there
    ss << "0";
    EXPECT_THROW(cb::from_hex(ss.str()), std::overflow_error);
}

TEST(Hex, InputStringData) {
    const std::string value{"0123456789abcdef"};
    const uint64_t expected{0x0123456789abcdefULL};
    EXPECT_EQ(expected, cb::from_hex(value));
}

TEST(Hex, ToHexUint8) {
    uint8_t val = uint8_t(-1);
    EXPECT_EQ("0xff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x00", cb::to_hex(val));
}

TEST(Hex, ToHexUint16) {
    uint16_t val = uint16_t(-1);
    EXPECT_EQ("0xffff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x0000", cb::to_hex(val));
}

TEST(Hex, ToHexUint32) {
    uint32_t val = uint32_t(-1);
    EXPECT_EQ("0xffffffff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x00000000", cb::to_hex(val));
}

TEST(Hex, ToHexUint64) {
    uint64_t val = uint64_t(-1);
    EXPECT_EQ("0xffffffffffffffff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x0000000000000000", cb::to_hex(val));
}

TEST(Hex, ToHexByteBuffer) {
    std::vector<uint8_t> buffer(4);
    for (auto& c : buffer) {
        c = 0xa5;
    }
    EXPECT_EQ("0xa5 0xa5 0xa5 0xa5",
              cb::to_hex({buffer.data(), buffer.size()}));
}
