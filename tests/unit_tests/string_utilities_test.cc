/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <platform/byte_literals.h>
#include <platform/string_utilities.h>

TEST(Size2Human, NoConversion) {
    EXPECT_EQ("2B", cb::size2human(2));
    EXPECT_EQ("2", cb::size2human(2, {}));
}

TEST(Size2Human, Conversion) {
    EXPECT_EQ("20MB", cb::size2human(20 * 1024 * 1024));
}

TEST(Human2Size, NoConversion) {
    EXPECT_EQ(2, cb::human2size("2"));
    EXPECT_EQ(2, cb::human2size("2B"));
}

TEST(Human2Size, Conversion) {
    EXPECT_EQ(32_KiB, cb::human2size("32k"));
    EXPECT_EQ(20_MiB, cb::human2size("20Mb"));
    EXPECT_EQ(3_GiB, cb::human2size("3G"));
    EXPECT_EQ(7_TiB, cb::human2size("7T"));
    EXPECT_EQ(8_TiB * 1024, cb::human2size("8PB"));
}

TEST(ToUpper, EmptyString) {
    EXPECT_EQ("", cb::toupper(""));
}

TEST(ToUpper, AlreadyUppercase) {
    EXPECT_EQ("HELLO", cb::toupper("HELLO"));
}

TEST(ToUpper, Lowercase) {
    EXPECT_EQ("HELLO", cb::toupper("hello"));
}

TEST(ToUpper, MixedCase) {
    EXPECT_EQ("HELLO WORLD", cb::toupper("HeLLo WoRLd"));
}

TEST(ToUpper, WithNumbers) {
    EXPECT_EQ("ABC123DEF", cb::toupper("abc123def"));
}

TEST(ToUpper, WithSpecialCharacters) {
    EXPECT_EQ("HELLO-WORLD_123!", cb::toupper("hello-world_123!"));
}

TEST(ToLower, EmptyString) {
    EXPECT_EQ("", cb::tolower(""));
}

TEST(ToLower, AlreadyLowercase) {
    EXPECT_EQ("hello", cb::tolower("hello"));
}

TEST(ToLower, Uppercase) {
    EXPECT_EQ("hello", cb::tolower("HELLO"));
}

TEST(ToLower, MixedCase) {
    EXPECT_EQ("hello world", cb::tolower("HeLLo WoRLd"));
}

TEST(ToLower, WithNumbers) {
    EXPECT_EQ("abc123def", cb::tolower("ABC123DEF"));
}

TEST(ToLower, WithSpecialCharacters) {
    EXPECT_EQ("hello-world_123!", cb::tolower("HELLO-WORLD_123!"));
}
