/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <folly/portability/GTest.h>
#include <platform/string_hex.h>
#include <limits>

class FromHexTest : public ::testing::Test {
protected:
    void expectFromHex(std::string_view input, uint64_t expected) {
        EXPECT_EQ(expected, cb::from_hex(input));
    }

    void expectThrowsInvalidArgument(std::string_view input) {
        EXPECT_THROW(cb::from_hex(input), std::invalid_argument);
    }

    void expectThrowsOverflow(std::string_view input) {
        EXPECT_THROW(cb::from_hex(input), std::overflow_error);
    }
};

// Happy case tests - valid hex strings
TEST_F(FromHexTest, SimpleHexString) {
    expectFromHex("1", 0x1);
    expectFromHex("10", 0x10);
    expectFromHex("ff", 0xff);
    expectFromHex("FF", 0xFF);
    expectFromHex("deadbeef", 0xdeadbeef);
}

TEST_F(FromHexTest, HexWithPrefix) {
    expectFromHex("0x1", 0x1);
    expectFromHex("0x10", 0x10);
    expectFromHex("0xff", 0xff);
    expectFromHex("0xFF", 0xFF);
    expectFromHex("0xdeadbeef", 0xdeadbeef);
}

TEST_F(FromHexTest, MixedCaseHex) {
    expectFromHex("DeAdBeEf", 0xdeadbeef);
    expectFromHex("0xDEadBEef", 0xdeadbeef);
    expectFromHex("aAbBcCdDeEfF", 0xaabbccddeeff);
}

TEST_F(FromHexTest, AllDigitsAndLetters) {
    expectFromHex("0123456789abcdef", 0x0123456789abcdef);
    expectFromHex("0x0123456789ABCDEF", 0x0123456789abcdef);
}

TEST_F(FromHexTest, LeadingZeros) {
    expectFromHex("0000", 0x0);
    expectFromHex("00ff", 0xff);
    expectFromHex("0x00deadbeef", 0xdeadbeef);
    expectFromHex("0x0000000000000001", 0x1);
}

TEST_F(FromHexTest, MinimumValue) {
    expectFromHex("0", 0x0);
    expectFromHex("0x0", 0x0);
    expectFromHex("00", 0x0);
}

TEST_F(FromHexTest, MaximumValue) {
    constexpr auto max_val = std::numeric_limits<uint64_t>::max();
    expectFromHex("ffffffffffffffff", max_val);
    expectFromHex("FFFFFFFFFFFFFFFF", max_val);
    expectFromHex("0xffffffffffffffff", max_val);
    expectFromHex("0xFFFFFFFFFFFFFFFF", max_val);
}

TEST_F(FromHexTest, SingleDigits) {
    for (int i = 0; i <= 9; ++i) {
        expectFromHex(std::to_string(i), i);
    }
    expectFromHex("a", 10);
    expectFromHex("b", 11);
    expectFromHex("c", 12);
    expectFromHex("d", 13);
    expectFromHex("e", 14);
    expectFromHex("f", 15);
}

// Edge cases and error conditions
TEST_F(FromHexTest, EmptyString) {
    expectThrowsInvalidArgument("");
}

TEST_F(FromHexTest, OnlyPrefix) {
    expectThrowsInvalidArgument("0x");
}

TEST_F(FromHexTest, InvalidCharacters) {
    expectThrowsInvalidArgument("g");
    expectThrowsInvalidArgument("z");
    expectThrowsInvalidArgument("0xg");
    expectThrowsInvalidArgument("0xgg");
    expectThrowsInvalidArgument("xyz");
    expectThrowsInvalidArgument("12g5");
}

TEST_F(FromHexTest, SpecialCharactersInvalid) {
    expectThrowsInvalidArgument(" ");
    expectThrowsInvalidArgument("0x ");
    expectThrowsInvalidArgument(" 0xff");
    expectThrowsInvalidArgument("0x ff");
    expectThrowsInvalidArgument("0xff ");
    expectThrowsInvalidArgument("\t");
    expectThrowsInvalidArgument("\n");
    expectThrowsInvalidArgument("+");
    expectThrowsInvalidArgument("-");
    expectThrowsInvalidArgument(".");
}

TEST_F(FromHexTest, OverflowBeyondUint64Max) {
    // 17 hex digits (more than 64 bits)
    expectThrowsOverflow("10000000000000000");
    expectThrowsOverflow("0x10000000000000000");
    // Add any digit to max value
    expectThrowsOverflow("1ffffffffffffffff");
    expectThrowsOverflow("0x1ffffffffffffffff");
    // Very long strings
    expectThrowsOverflow("ffffffffffffffff0");
    expectThrowsOverflow("0xffffffffffffffff0");
}

TEST_F(FromHexTest, UppercasePrefixIsInvalid) {
    // Only lowercase "0x" prefix is supported
    expectThrowsInvalidArgument("0XdEaDbEeF");
    expectThrowsInvalidArgument("0XFF");
}

TEST_F(FromHexTest, MultipleZeros) {
    expectFromHex("000000000000", 0x0);
    expectFromHex("0x000000000000", 0x0);
}

TEST_F(FromHexTest, PowersOfTwo) {
    expectFromHex("1", 0x1);
    expectFromHex("2", 0x2);
    expectFromHex("4", 0x4);
    expectFromHex("8", 0x8);
    expectFromHex("10", 0x10);
    expectFromHex("100", 0x100);
    expectFromHex("1000", 0x1000);
    expectFromHex("8000000000000000", 0x8000000000000000);
}

TEST_F(FromHexTest, AllLettersSequence) {
    expectFromHex("abcdef", 0xabcdef);
    expectFromHex("ABCDEF", 0xabcdef);
}

TEST_F(FromHexTest, AlternatingPattern) {
    expectFromHex("aaaaaaaa", 0xaaaaaaaa);
    expectFromHex("55555555", 0x55555555);
    expectFromHex("0xaaaaaaaaaaaaaaaa", 0xaaaaaaaaaaaaaaaa);
    expectFromHex("0x5555555555555555", 0x5555555555555555);
}

TEST_F(FromHexTest, PrefixWithLeadingZeros) {
    expectFromHex("0x0000000000000001", 0x1);
    expectFromHex("0x00000000000000ff", 0xff);
    expectFromHex("0xffffffffffffffff", 0xffffffffffffffff);
}

// Negative input tests
TEST_F(FromHexTest, NegativeSignSimple) {
    expectThrowsInvalidArgument("-1");
    expectThrowsInvalidArgument("-ff");
    expectThrowsInvalidArgument("-deadbeef");
}

TEST_F(FromHexTest, NegativeSignWithPrefix) {
    expectThrowsInvalidArgument("-0x1");
    expectThrowsInvalidArgument("-0xff");
    expectThrowsInvalidArgument("-0xdeadbeef");
}

TEST_F(FromHexTest, NegativeSignEdgeCases) {
    expectThrowsInvalidArgument("-0");
    expectThrowsInvalidArgument("-0x0");
    expectThrowsInvalidArgument("-a");
    expectThrowsInvalidArgument("-f");
}

TEST_F(FromHexTest, NegativeSignWithMaxValue) {
    expectThrowsInvalidArgument("-ffffffffffffffff");
    expectThrowsInvalidArgument("-0xffffffffffffffff");
}

TEST_F(FromHexTest, MultipleMinusSigns) {
    expectThrowsInvalidArgument("--1");
    expectThrowsInvalidArgument("--0xff");
    expectThrowsInvalidArgument("---deadbeef");
}

TEST_F(FromHexTest, MinusSignWithLeadingZeros) {
    expectThrowsInvalidArgument("-0000");
    expectThrowsInvalidArgument("-00ff");
    expectThrowsInvalidArgument("-0x00deadbeef");
}
