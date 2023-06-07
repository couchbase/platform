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
#include <stdexcept>

using cb::compression::Allocator;
using cb::compression::Buffer;
using cb::compression::deflateSnappy;
using cb::compression::get_uncompressed_length;
using cb::compression::inflateSnappy;

TEST(Compression, TestSnappyCompression) {
    Buffer input;
    Buffer output(Allocator{Allocator::Mode::Malloc});

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_TRUE(deflateSnappy(input, output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());
    auto def = deflateSnappy(input);
    EXPECT_EQ(output.size(), def->length());
    EXPECT_EQ(0, std::memcmp(output.data(), def->data(), def->length()));

    Buffer back;
    EXPECT_TRUE(inflateSnappy(output, back));

    EXPECT_EQ(8192u, back.size());
    EXPECT_NE(nullptr, back.data());
    EXPECT_EQ(0, memcmp(input.data(), back.data(), input.size()));
    auto inf = inflateSnappy(output);
    EXPECT_EQ(input.size(), inf->length());
    EXPECT_EQ(0, memcmp(input.data(), inf->data(), input.size()));

    // Verify that we don't exceed the max size:
    EXPECT_FALSE(inflateSnappy(output, back, 4096));
    try {
        auto ret = inflateSnappy(output, 4096);
        FAIL() << "Should not allow inflate of such a big blob";
    } catch (const std::range_error& e) {
        EXPECT_STREQ(
                "cb::compression::inflate(): Inflated length 8192 would exceed "
                "max: 4096",
                e.what());
    }
}

TEST(Compression, TestIllegalSnappyInflate) {
    Buffer input;
    Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);

    EXPECT_FALSE(inflateSnappy(input, output));
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
    Buffer input;
    Buffer output;

    input.resize(8192);
    memset(input.data(), 'a', 8192);
    EXPECT_TRUE(deflateSnappy({input.data(), input.size()}, output));
    EXPECT_LT(output.size(), 8192u);
    EXPECT_NE(nullptr, output.data());

    EXPECT_EQ(8192u,
              get_uncompressed_length(folly::io::CodecType::SNAPPY,
                                      {output.data(), output.size()}));
}
