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

#include <gtest/gtest.h>
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

TEST(SizedBufferTest, Access) {
    const char s[] = "Hello, World!";
    auto A = make_ccb(s);

    EXPECT_EQ(s, A.data());
    EXPECT_EQ(s, A.begin());
    EXPECT_EQ(s, A.cbegin());
    EXPECT_EQ(&s[sizeof(s) - 1], A.end());
    EXPECT_EQ(&s[sizeof(s) - 1], A.cend());

    EXPECT_EQ('H', A.front());
    EXPECT_EQ('!', A.back());

    for (size_t i = 0; i < sizeof(s) - 1; ++i) {
        EXPECT_EQ(s[i], A[i]);
        EXPECT_EQ(s[i], A.at(i));
    }
    EXPECT_THROW(A.at(A.size()), std::out_of_range);

    cb::const_char_buffer B;
    EXPECT_THROW(B.at(0), std::out_of_range);
}

TEST(SizedBufferTest, Capacity) {
    auto A = make_ccb("Hello, World!");
    EXPECT_EQ(13, A.size());
    EXPECT_FALSE(A.empty());
    EXPECT_TRUE(make_ccb("").empty());
}

TEST(SizedBufferTest, Find) {
    auto A = make_ccb("Hello, World!");
    EXPECT_EQ(0, A.find(make_ccb("Hello")));
    EXPECT_EQ(7, A.find(make_ccb("World!")));
    EXPECT_EQ(A.npos, A.find(make_ccb("Trond!")));
    EXPECT_EQ(0, A.find(make_ccb("")));

    auto R = make_ccb("RepeatRepeatRepeat");
    EXPECT_EQ(0, R.find(make_ccb("Repeat")));
    EXPECT_EQ(6, R.find(make_ccb("Repeat"), 1));
    EXPECT_EQ(12, R.find(make_ccb("Repeat"), 7));

    cb::const_char_buffer B;
    EXPECT_EQ(B.npos, B.find(make_ccb("")));
}

TEST(SizedBufferTest, FindFirstOf) {
    auto A = make_ccb("Hello, World!");
    EXPECT_EQ(0, A.find_first_of(make_ccb("Hello")));
    EXPECT_EQ(1, A.find_first_of(make_ccb("ello")));
    EXPECT_EQ(2, A.find_first_of(make_ccb("llo")));
    EXPECT_EQ(2, A.find_first_of(make_ccb("lo")));
    EXPECT_EQ(4, A.find_first_of(make_ccb("o")));
    EXPECT_EQ(8, A.find_first_of(make_ccb("o"), 6));
    EXPECT_EQ(12, A.find_first_of(make_ccb("!")));
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

TEST(SizedBufferTest, FromVector) {
    std::string str = "Hello, World";
    std::vector<char> vec(str.begin(), str.end());
    cb::const_char_buffer ccb = vec;
    cb::char_buffer cb = vec;

    EXPECT_EQ(vec.data(), ccb.data());
    EXPECT_EQ(vec.size(), ccb.size());
    EXPECT_EQ(vec.data(), cb.data());
    EXPECT_EQ(vec.size(), cb.size());
}

TEST(SizedBufferTest, ToConst) {
    char str[] = "Hello, World!";
    cb::char_buffer cb{str, sizeof(str) - 1};
    cb::const_char_buffer ccb = cb;

    EXPECT_EQ(ccb.data(), cb.data());
    EXPECT_EQ(ccb.size(), cb.size());
}

TEST(SizedBufferTest, cString1) {
    const char* cStr = "Hello, World!";
    cb::const_char_buffer ccb(cStr);

    EXPECT_STREQ("Hello, World!", ccb.data());
    EXPECT_EQ(std::strlen("Hello, World!"), ccb.size());

    auto str = cb::to_string(ccb);
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
