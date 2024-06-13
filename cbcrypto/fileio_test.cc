/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */
#include <cbcrypto/common.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/file_writer.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <filesystem>

using namespace cb::crypto;
using namespace std::string_view_literals;

class FileIoTest : public ::testing::Test {
protected:
    void SetUp() override {
        file = cb::io::mktemp("FileIoTest");
    }
    void TearDown() override {
        remove(file);
    }

    std::filesystem::path file;
};

// Write a file, but we don't have any encyption keys
TEST_F(FileIoTest, FileWriterTestPlain) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file);
    EXPECT_FALSE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();
    EXPECT_EQ(content, cb::io::loadFile(file.generic_string()));
}

TEST_F(FileIoTest, FileWriterTestEncrypted) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create(DataEncryptionKey::generate(), file);
    EXPECT_TRUE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();
    auto data = cb::io::loadFile(file.generic_string());
    EXPECT_EQ(0, data.find("\0CEF\0"));
}

TEST_F(FileIoTest, ReadFile) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file);
    EXPECT_FALSE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();
    EXPECT_EQ(content, cb::io::loadFile(file.generic_string()));

    auto reader = FileReader::create(
            file,
            [](auto) -> std::shared_ptr<DataEncryptionKey> { return {}; });
    EXPECT_FALSE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());
}

TEST_F(FileIoTest, ReadFileEncrypted) {
    const std::string_view content = "This is the content"sv;
    auto key = DataEncryptionKey::generate();
    EXPECT_TRUE(key);
    auto writer = FileWriter::create(key, file);
    EXPECT_TRUE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();

    auto lookup = [&key](auto k) -> std::shared_ptr<DataEncryptionKey> {
        if (key && key->id == k) {
            return key;
        }
        return {};
    };

    auto reader = FileReader::create(file, lookup);
    EXPECT_TRUE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());

    // verify that we can't read the file if we don't have the key
    key.reset();
    try {
        FileReader::create(file, lookup);
        FAIL() << "We should not be able to decode the file without the key";
    } catch (const std::exception& error) {
        EXPECT_NE(nullptr, strstr(error.what(), "Missing key"));
    }
}

TEST_F(FileIoTest, BufferedFileWriterTestEncrypted) {
    auto key = DataEncryptionKey::generate();
    auto lookup = [&key](auto k) -> std::shared_ptr<DataEncryptionKey> {
        if (key && key->id == k) {
            return key;
        }
        return {};
    };

    auto writer = FileWriter::create(key, file, 100);
    EXPECT_TRUE(writer->is_encrypted());

    // Write a single character 10 times
    for (int ii = 0; ii < 10; ++ii) {
        writer->write(std::string_view{"a", 1});
    }
    // Write a bigger chunk which exceeds the buffer size which would cause
    // us to generate a new chunk
    writer->write(std::string(101, 'a'));
    writer->flush();
    writer.reset();

    auto reader = FileReader::create(file, lookup);
    auto chunk = reader->nextChunk();
    EXPECT_EQ(10, chunk.size());
    chunk = reader->nextChunk();
    EXPECT_EQ(101, chunk.size());
}
