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

#include <gtest/gtest.h>
#include <platform/compress.h>
#include <strings.h>

TEST(Compression, DetectInvalidAlgoritm) {
    cb::compression::Buffer buffer;
    EXPECT_THROW(cb::compression::inflate(
                     (cb::compression::Algorithm)5, {}, buffer),
                 std::invalid_argument);
    EXPECT_THROW(cb::compression::deflate(
                     (cb::compression::Algorithm)5, {}, buffer),
                 std::invalid_argument);
}

TEST(Compression, TestCompression) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    output.len = 16000;
    input.data.reset(new char[8192]);
    memset(input.data.get(), 'a', 8192);

    EXPECT_TRUE(cb::compression::deflate(
        cb::compression::Algorithm::Snappy, {input.data.get(), 8192},
        output));
    EXPECT_LT(output.len, 8192);
    EXPECT_NE(nullptr, output.data.get());

    cb::compression::Buffer back;
    EXPECT_TRUE(cb::compression::inflate(cb::compression::Algorithm::Snappy,
                                         {output.data.get(), output.len},
                                         back));
    EXPECT_EQ(8192, back.len);
    EXPECT_NE(nullptr, back.data.get());
    EXPECT_EQ(0, memcmp(input.data.get(), back.data.get(), 8192));

    // Verify that we don't exceed the max size:
    EXPECT_FALSE(cb::compression::inflate(cb::compression::Algorithm::Snappy,
                                          {output.data.get(), output.len},
                                          back,
                                          4096));
}

TEST(Compression, TestIllegalInflate) {
    cb::compression::Buffer input;
    cb::compression::Buffer output;

    output.len = 16000;
    input.data.reset(new char[8192]);
    memset(input.data.get(), 'a', 8192);

    EXPECT_FALSE(cb::compression::inflate(
        cb::compression::Algorithm::Snappy, {input.data.get(), 8192}, output));
}
