/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
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
}

TEST(Compression, ToAlgorithm) {
    using namespace cb::compression;
    EXPECT_EQ(Algorithm::Snappy, to_algorithm("SnApPy"));
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
