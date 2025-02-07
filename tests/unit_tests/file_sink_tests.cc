/*
 *     Copyright 2025-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <folly/portability/GTest.h>
#include <gsl/gsl-lite.hpp>
#include <platform/dirutils.h>
#include <platform/file_sink.h>

namespace cb::io {
class FileSinkTest : public ::testing::Test {
protected:
    void TearDown() override {
        if (exists(path)) {
            remove(path);
        }
    }

    std::filesystem::path path{"testfile.txt"};
};

TEST_F(FileSinkTest, ConstructorOpensFile) {
    FileSink obj(path);
    obj.close();
    EXPECT_TRUE(exists(path));
}

#ifndef WIN32
TEST_F(FileSinkTest, ConstructorThrowsOnInvalidPath) {
    EXPECT_THROW(
            FileSink("/invalid/path/testfile.txt", FileSink::Mode::Truncate),
            std::system_error);
}
#endif

TEST_F(FileSinkTest, SinkWritesData) {
    FileSink sink(path);
    const std::string data = "Hello, World!";
    sink.sink(data);
    sink.close();
    EXPECT_EQ(sink.getBytesWritten(), data.size());
    auto written = loadFile(path);
    EXPECT_EQ(data, written);
}

TEST_F(FileSinkTest, SinkThrowsAfterClose) {
    FileSink sink(path);
    sink.close();
    std::string data = "Hello, World!";
    EXPECT_THROW(sink.sink(data), gsl::fail_fast);
}

TEST_F(FileSinkTest, FsyncFlushesData) {
    FileSink sink(path);
    std::string data = "Hello, World!";
    sink.sink(data);
    EXPECT_NO_THROW(sink.fsync());
    auto written = loadFile(path);
    EXPECT_EQ(data, written);
}

TEST_F(FileSinkTest, CloseFlushesData) {
    FileSink sink(path);
    std::string data = "Hello, World!";
    sink.sink(data);
    EXPECT_NO_THROW(sink.close());
    EXPECT_EQ(sink.getBytesWritten(), data.size());
    auto written = loadFile(path);
    EXPECT_EQ(data, written);
}

TEST_F(FileSinkTest, AppendAppendsData) {
    FileSink sink(path);
    std::string data = "Hello, World!";
    sink.sink(data);
    EXPECT_NO_THROW(sink.close());
    FileSink sink2(path, FileSink::Mode::Append);
    std::string appended = "Have you ever seen the rain?";
    sink2.sink(appended);
    sink2.close();
    auto written = loadFile(path);
    EXPECT_EQ(data + appended, written);
}
} // namespace cb::io
