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
#include <strings.h>
#include <platform/compress.h>

TEST(Compression, DetectInvalidAlgoritm) {
    using namespace Couchbase;
    Compression::Buffer buffer;
    EXPECT_THROW(
        Compression::inflate((Compression::Algorithm)5, nullptr, 0, buffer),
        std::invalid_argument);
    EXPECT_THROW(
        Compression::deflate((Compression::Algorithm)5, nullptr, 0, buffer),
        std::invalid_argument);
}

TEST(Compression, TestCompression) {
    using namespace Couchbase;
    Compression::Buffer input;
    Compression::Buffer output;

    output.len = 16000;
    input.data.reset(new char[8192]);
    memset(input.data.get(), 'a', 8192);

    EXPECT_TRUE(Compression::deflate(Compression::Algorithm::Snappy,
                                     input.data.get(),
                                     8192, output));
    EXPECT_LT(output.len, 8192);
    EXPECT_NE(nullptr, output.data.get());

    Compression::Buffer back;

    EXPECT_TRUE(Compression::inflate(Compression::Algorithm::Snappy,
                                     output.data.get(),
                                     output.len, back));
    EXPECT_EQ(8192, back.len);
    EXPECT_NE(nullptr, back.data.get());
    EXPECT_EQ(0, memcmp(input.data.get(), back.data.get(), 8192));
}

TEST(Compression, TestIllegalInflate) {
    using namespace Couchbase;
    Compression::Buffer input;
    Compression::Buffer output;

    output.len = 16000;
    input.data.reset(new char[8192]);
    memset(input.data.get(), 'a', 8192);

    EXPECT_FALSE(Compression::inflate(Compression::Algorithm::Snappy,
                                     input.data.get(),
                                     8192, output));
}
