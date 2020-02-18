/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
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

#include <folly/portability/GTest.h>
#include <platform/sized_buffer.h>

#include <unordered_set>

template <size_t N>
cb::const_char_buffer make_ccb(const char (&c)[N]) {
    return {c, N - 1};
}

TEST(SizedBufferTest, Comparison) {
    auto A = "abc"_ccb;
    auto B = "def"_ccb;

    // Technically all these could be done through the appropriate
    // EXPECT_* macros but I feel it's more explicit to invoke the
    // operator overloads directly.

    EXPECT_TRUE(A == A);
    EXPECT_TRUE(A >= A);
    EXPECT_TRUE(A <= A);
    EXPECT_TRUE(B == B);
    EXPECT_TRUE(B >= B);
    EXPECT_TRUE(B <= B);

    EXPECT_FALSE(A == B);
    EXPECT_TRUE(A != B);
    EXPECT_TRUE(A < B);
    EXPECT_TRUE(A <= B);
    EXPECT_FALSE(A >= B);
    EXPECT_FALSE(A > B);

    EXPECT_FALSE(B == A);
    EXPECT_TRUE(B != A);
    EXPECT_FALSE(B < A);
    EXPECT_FALSE(B <= A);
    EXPECT_TRUE(B >= A);
    EXPECT_TRUE(B > A);

    auto C = make_ccb("abc");
    auto D = make_ccb("abcd");

    EXPECT_FALSE(C == D);
    EXPECT_TRUE(C != D);
    EXPECT_TRUE(D >= C);
    EXPECT_TRUE(D > C);
    EXPECT_FALSE(D <= C);
    EXPECT_FALSE(D < C);

    // Empty buffers

    auto E = make_ccb("");
    cb::const_char_buffer F;
    EXPECT_TRUE(E == F);
    EXPECT_FALSE(F == C);
}

TEST(SizedBufferTest, SubStr) {
    auto A = make_ccb("Hello, World!");

    EXPECT_EQ(make_ccb("Hello, World!"), A.substr(0));
    EXPECT_EQ(make_ccb("Hello"), A.substr(0, 5));
    EXPECT_EQ(make_ccb("World"), A.substr(7, 5));
    EXPECT_EQ(make_ccb("World!"), A.substr(7, 50));
    EXPECT_EQ(make_ccb("World!"), A.substr(7));
    EXPECT_EQ(make_ccb(""), A.substr(0, 0));

    EXPECT_THROW(A.substr(A.size() + 1), std::out_of_range);
    EXPECT_THROW(A.substr(50), std::out_of_range);

    cb::const_char_buffer B;
    EXPECT_EQ(B, B.substr(0, 50));
    EXPECT_THROW(A.substr(50), std::out_of_range);
}

TEST(SizedBufferTest, Capacity) {
    auto A = make_ccb("Hello, World!");
    EXPECT_EQ(13u, A.size());
    EXPECT_FALSE(A.empty());
    EXPECT_TRUE(make_ccb("").empty());
}

TEST(SizedBufferTest, FindFirstOf) {
    auto A = make_ccb("Hello, World!");
    EXPECT_EQ(0u, A.find_first_of(make_ccb("Hello")));
    EXPECT_EQ(1u, A.find_first_of(make_ccb("ello")));
    EXPECT_EQ(2u, A.find_first_of(make_ccb("llo")));
    EXPECT_EQ(2u, A.find_first_of(make_ccb("lo")));
    EXPECT_EQ(4u, A.find_first_of(make_ccb("o")));
    EXPECT_EQ(8u, A.find_first_of(make_ccb("o"), 6));
    EXPECT_EQ(12u, A.find_first_of(make_ccb("!")));
    EXPECT_EQ(A.npos, A.find_first_of(make_ccb("?")));
    EXPECT_EQ(A.npos, A.find_first_of(make_ccb("")));
    EXPECT_EQ(A.npos, A.find_first_of(make_ccb("H"), 5));

    cb::const_char_buffer B;
    EXPECT_EQ(B.npos, B.find_first_of(make_ccb("")));
    EXPECT_EQ(B.npos, B.find_first_of(make_ccb("abcdef"), 1));
    EXPECT_EQ(B.npos, B.find_first_of(make_ccb("?")));
}

// Smoke test that hashing and comparison works well enough for unordered_set
TEST(SizedBufferTest, Set) {
    std::unordered_set<cb::const_char_buffer> s;

    auto ret = s.insert(make_ccb("Hello, World!"));
    EXPECT_TRUE(ret.second);

    ret = s.insert(make_ccb("Hello, World"));
    EXPECT_TRUE(ret.second);

    ret = s.insert(make_ccb("Hello"));
    EXPECT_TRUE(ret.second);

    ret = s.insert(make_ccb("World"));
    EXPECT_TRUE(ret.second);

    ret = s.insert(make_ccb("Hello, World!"));
    EXPECT_FALSE(ret.second);
}

TEST(SizedBufferTest, FromString) {
    std::string str = "Hello, World";
    cb::const_char_buffer ccb = str;
    cb::char_buffer cb = str;
    EXPECT_EQ(str.data(), ccb.data());
    EXPECT_EQ(str.size(), ccb.size());
    EXPECT_EQ(str.data(), cb.data());
    EXPECT_EQ(str.size(), cb.size());
}

TEST(SizedBufferTest, cString1) {
    const char* cStr = "Hello, World!";
    cb::const_char_buffer ccb(cStr);

    EXPECT_STREQ("Hello, World!", ccb.data());
    EXPECT_EQ(std::strlen("Hello, World!"), ccb.size());

    auto str = std::string(ccb);
    EXPECT_EQ(0, str.compare(ccb.data()));
    EXPECT_EQ(ccb.size(), str.size());
}

TEST(SizedBufferTest, cString2) {
    const char* cStr = "Hello, World!";
    std::string str(cStr);
    cb::const_char_buffer ccb1(str);
    cb::const_char_buffer ccb2(cStr);
    EXPECT_EQ(ccb1, ccb2);
}
