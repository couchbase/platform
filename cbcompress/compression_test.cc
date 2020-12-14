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

#include <folly/portability/GTest.h>
#include <platform/compress.h>

TEST(Compression, DetectInvalidAlgoritm) {
    cb::compression::Buffer buffer;
    EXPECT_THROW(cb::compression::inflate(
                     (cb::compression::Algorithm)5, {}, buffer),
                 std::invalid_argument);
    EXPECT_THROW(cb::compression::deflate(
                     (cb::compression::Algorithm)5, {}, buffer),
                 std::invalid_argument);
}

TEST(Compression, TestSnappyCompression) {
    cb::compression::Buffer input;
    cb::compression::Buffer output(cb::compression::Allocator{
            cb::compression::Allocator::Mode::Malloc});

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_TRUE(cb::compression::deflate(
            cb::compression::Algorithm::Snappy, input, output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());

    cb::compression::Buffer back;
    EXPECT_TRUE(cb::compression::inflate(
            cb::compression::Algorithm::Snappy, output, back));
    EXPECT_EQ(8192u, back.size());
    EXPECT_NE(nullptr, back.data());
    EXPECT_EQ(0, memcmp(input.data(), back.data(), input.size()));

    // Verify that we don't exceed the max size:
    EXPECT_FALSE(cb::compression::inflate(
            cb::compression::Algorithm::Snappy, output, back, 4096));
}

TEST(Compression, TestIllegalSnappyInflate) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_FALSE(cb::compression::inflate(
            cb::compression::Algorithm::Snappy, input, output));
}

TEST(Compression, ToString) {
    EXPECT_EQ("Snappy", to_string(cb::compression::Algorithm::Snappy));
    EXPECT_EQ("LZ4", to_string(cb::compression::Algorithm::LZ4));
}

TEST(Compression, ToAlgorithm) {
    using namespace cb::compression;
    EXPECT_EQ(Algorithm::Snappy, to_algorithm("SnApPy"));
    EXPECT_EQ(Algorithm::LZ4, to_algorithm("lz4"));
    EXPECT_THROW(to_algorithm("foo"), std::invalid_argument);
}

TEST(Compression, TestGetUncompressedLength) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);
    EXPECT_TRUE(cb::compression::deflate(cb::compression::Algorithm::Snappy,
                                         {input.data(), input.size()},
                                         output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());

    EXPECT_EQ(8192u,
              cb::compression::get_uncompressed_length(
                      cb::compression::Algorithm::Snappy,
                      {output.data(), output.size()}));
}

#ifdef CB_LZ4_SUPPORT
TEST(Compression, TestLZ4Compression) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_TRUE(cb::compression::deflate(
            cb::compression::Algorithm::LZ4, input, output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());

    cb::compression::Buffer back;
    EXPECT_TRUE(cb::compression::inflate(
            cb::compression::Algorithm::LZ4, output, back));
    EXPECT_EQ(8192u, back.size());
    EXPECT_NE(nullptr, back.data());
    EXPECT_EQ(0, memcmp(input.data(), back.data(), input.size()));
}

TEST(Compression, TestIllegalLZ4Inflate) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);
    *reinterpret_cast<uint32_t*>(input.data()) = 512;

    EXPECT_FALSE(cb::compression::inflate(
            cb::compression::Algorithm::LZ4, input, output));
}

TEST(Compression, TestLZ4GetUncompressedLength) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_TRUE(cb::compression::deflate(cb::compression::Algorithm::LZ4,
                                         {input.data(), input.size()},
                                         output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());

    EXPECT_EQ(8192u,
              cb::compression::get_uncompressed_length(
                      cb::compression::Algorithm::LZ4,
                      {output.data(), output.size()}));
}

#endif
