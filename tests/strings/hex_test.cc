/*
 *     Copyright 2017-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/string_hex.h>

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

TEST(Hex, InputWithPrefix) {
    EXPECT_EQ(0x4096, cb::from_hex("0x4096"));
}

TEST(Hex, ToHexUint8) {
    auto val = uint8_t(-1);
    EXPECT_EQ("0xff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x00", cb::to_hex(val));
}

TEST(Hex, ToHexUint16) {
    auto val = uint16_t(-1);
    EXPECT_EQ("0xffff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x0000", cb::to_hex(val));
}

TEST(Hex, ToHexUint32) {
    auto val = uint32_t(-1);
    EXPECT_EQ("0xffffffff", cb::to_hex(val));
    val = 0;
    EXPECT_EQ("0x00000000", cb::to_hex(val));
}

TEST(Hex, ToHexUint64) {
    auto val = uint64_t(-1);
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

TEST(Hex, HexEncode) {
    std::vector<uint8_t> buffer = {{0xde, 0xad, 0xbe, 0xef, 0xff}};
    EXPECT_EQ("deadbeefff", cb::hex_encode({buffer.data(), buffer.size()}));
    EXPECT_EQ("deadbeefff",
              cb::hex_encode({reinterpret_cast<const char*>(buffer.data()),
                              buffer.size()}));
}
