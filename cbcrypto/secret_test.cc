/*
 *     Copyright 2026-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <cbcrypto/secret.h>
#include <folly/portability/GTest.h>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using namespace cb::crypto;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(Secret, DefaultConstruction) {
    SecretString s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0u, s.size());
}

TEST(Secret, ConstructFromContainer) {
    std::string raw{"p@ssw0rd"};
    SecretString s{raw};
    EXPECT_EQ(std::string_view{"p@ssw0rd"}, static_cast<std::string_view>(s));
    EXPECT_EQ(8u, s.size());
}

TEST(Secret, ConstructFromRvalueContainer) {
    std::string raw{"p@ssw0rd"};
    SecretString s{std::move(raw)};
    EXPECT_EQ(std::string_view{"p@ssw0rd"}, static_cast<std::string_view>(s));
}

TEST(Secret, ConstructFromStringView) {
    SecretString s{std::string_view{"secret"}};
    EXPECT_EQ(std::string_view{"secret"}, static_cast<std::string_view>(s));
    EXPECT_EQ(6u, s.size());
}

// ---------------------------------------------------------------------------
// Copy is disabled (compile-time only — checked by static_assert in practice)
// ---------------------------------------------------------------------------

TEST(Secret, CopyConstructorIsDeleted) {
    EXPECT_FALSE(std::is_copy_constructible_v<SecretString>);
}

TEST(Secret, CopyAssignmentIsDeleted) {
    EXPECT_FALSE(std::is_copy_assignable_v<SecretString>);
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST(Secret, MoveConstructionTransfersData) {
    SecretString src{std::string{"my_secret"}};
    SecretString dst{std::move(src)};

    EXPECT_EQ(std::string_view{"my_secret"},
              static_cast<std::string_view>(dst));
}

TEST(Secret, MoveAssignmentTransfersData) {
    SecretString src{std::string{"token_xyz"}};
    SecretString dst;
    dst = std::move(src);

    EXPECT_EQ(std::string_view{"token_xyz"},
              static_cast<std::string_view>(dst));
}

// ---------------------------------------------------------------------------
// Observers
// ---------------------------------------------------------------------------

TEST(Secret, DataAndSizeAreConsistent) {
    SecretString s{std::string{"hello"}};
    EXPECT_EQ(5u, s.size());
    EXPECT_EQ('h', s.data()[0]);
    EXPECT_FALSE(s.empty());
}

TEST(Secret, EmptyAfterDefaultConstruction) {
    SecretString s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0u, s.size());
}

// ---------------------------------------------------------------------------
// std::string_view conversion
// ---------------------------------------------------------------------------

TEST(Secret, ImplicitConversionToStringView) {
    SecretString s{std::string{"abc123"}};
    // Implicit: pass to a function that takes std::string_view.
    auto accept = [](std::string_view sv) { return sv; };
    std::string_view sv = accept(s);
    EXPECT_EQ("abc123", sv);
}

TEST(Secret, StringViewPointsIntoStoredData) {
    SecretString s{std::string{"key"}};
    std::string_view sv = s;
    EXPECT_EQ(static_cast<const void*>(s.data()),
              static_cast<const void*>(sv.data()));
    EXPECT_EQ(s.size(), sv.size());
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

TEST(Secret, ClearWipesAndEmptiesSecret) {
    SecretString s{std::string(8, 'X')};
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0u, s.size());
}

// ---------------------------------------------------------------------------
// SecretBytes (vector<unsigned char>)
// ---------------------------------------------------------------------------

TEST(SecretBytes, ConstructFromVector) {
    std::vector<unsigned char> key{0x01, 0x02, 0x03, 0x04};
    SecretBytes sb{key};
    ASSERT_EQ(4u, sb.size());
    EXPECT_EQ(0x01u, sb.data()[0]);
    EXPECT_EQ(0x04u, sb.data()[3]);
}

TEST(SecretBytes, StringViewConversionPreservesBytes) {
    std::vector<unsigned char> key{0xDE, 0xAD, 0xBE, 0xEF};
    SecretBytes sb{key};
    std::string_view sv = sb;
    ASSERT_EQ(4u, sv.size());
    EXPECT_EQ('\xDE', sv[0]);
    EXPECT_EQ('\xEF', sv[3]);
}
