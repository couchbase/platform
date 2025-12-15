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
#include <cbcrypto/encrypted_file_header.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/file_writer.h>
#include <folly/ScopeGuard.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <filesystem>

using namespace cb::crypto;
using namespace std::string_view_literals;
using namespace std::string_literals;

TEST(EncryptedFileHeaderTest, Iterations) {
    EncryptedFileHeader header(KeyDerivationKey::PasswordKeyId,
                               KeyDerivationMethod::PasswordBased,
                               Compression::None);
    EXPECT_EQ(1024U << 15, header.set_pbkdf_iterations(1024U << 16));
    EXPECT_EQ(1024U << 15, header.get_pbkdf_iterations());
    for (unsigned int ii = 0; ii <= 15; ++ii) {
        auto value = 1024U << ii;
        EXPECT_EQ(value, header.set_pbkdf_iterations(value));
        EXPECT_EQ(value, header.get_pbkdf_iterations());
    }
}

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
    EXPECT_EQ(content, cb::io::loadFile(file));
}

TEST_F(FileIoTest, FileWriterTestEncrypted) {
    const std::string_view content = "This is the content"sv;
    SharedKeyDerivationKey key = KeyDerivationKey::generate();
    auto writer = FileWriter::create(key, file);
    EXPECT_TRUE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();
    auto data = cb::io::loadFile(file);
    ASSERT_GE(data.size(), sizeof(EncryptedFileHeader));
    auto header = reinterpret_cast<const EncryptedFileHeader*>(data.data());
    EXPECT_TRUE(header->is_supported());
    EXPECT_TRUE(header->is_encrypted());
    EXPECT_EQ(Compression::None, header->get_compression());
    EXPECT_EQ(key->id, header->get_id());
}

/**
 * Try to write a text which compress very well in multiple chunks to
 * the file and read the entire file back as one chunk (which internally
 * would need to inflate and concatenate each chunk)
 *
 * @param file the file to operate on
 * @param compression the compression to use
 */
static void testEncryptedAndCompressed(const std::filesystem::path& file,
                                       Compression compression) {
    std::string content(8192, 'a');
    SharedKeyDerivationKey key = KeyDerivationKey::generate();
    auto writer = FileWriter::create(key, file, 8192, compression);
    EXPECT_TRUE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer->write(content);
    writer->close();
    writer.reset();
    // we wrote the content twice, so append the content to itself
    content.append(content);
    auto data = cb::io::loadFile(file);
    ASSERT_GE(data.size(), sizeof(EncryptedFileHeader));
    auto header = reinterpret_cast<const EncryptedFileHeader*>(data.data());
    EXPECT_TRUE(header->is_supported());
    EXPECT_TRUE(header->is_encrypted());
    EXPECT_EQ(compression, header->get_compression());
    EXPECT_EQ(key->id, header->get_id());
    EXPECT_LT(data.size(), content.size())
            << "Expected the content to be compressed";

    auto reader = FileReader::create(
            file, [&key](auto) -> SharedKeyDerivationKey { return key; });
    EXPECT_TRUE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());
}

TEST_F(FileIoTest, FileWriterTestEncryptedCompressedSnappy) {
    testEncryptedAndCompressed(file, Compression::Snappy);
}

TEST_F(FileIoTest, FileWriterTestEncryptedCompressedZlib) {
    testEncryptedAndCompressed(file, Compression::ZLIB);
}

TEST_F(FileIoTest, ReadFile) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file);
    EXPECT_FALSE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();
    EXPECT_EQ(content, cb::io::loadFile(file));

    auto reader = FileReader::create(
            file, [](auto) -> SharedKeyDerivationKey { return {}; });
    EXPECT_FALSE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());
}

static void testReadFileEncrypted(const std::filesystem::path& file,
                                  KeyDerivationMethod kdm) {
    const std::string_view content = "This is the content"sv;
    std::shared_ptr<KeyDerivationKey> key = KeyDerivationKey::generate();
    ASSERT_TRUE(key);
    key->derivationMethod = kdm;

    auto writer = FileWriter::create(key, file);
    EXPECT_TRUE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer.reset();

    auto lookup = [&key](auto k) -> SharedKeyDerivationKey {
        if (key && key->id == k) {
            return key;
        }
        return {};
    };

    auto reader = FileReader::create(file, lookup);
    EXPECT_TRUE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());

    // verify that we can read the file when the default key derivation method
    // is different
    key->derivationMethod = (kdm == KeyDerivationMethod::NoDerivation)
                                    ? KeyDerivationMethod::KeyBased
                                    : KeyDerivationMethod::NoDerivation;
    reader = FileReader::create(file, lookup);
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

TEST_F(FileIoTest, ReadFileEncrypted) {
    testReadFileEncrypted(file, KeyDerivationMethod::NoDerivation);
    testReadFileEncrypted(file, KeyDerivationMethod::KeyBased);
    testReadFileEncrypted(file, KeyDerivationMethod::PasswordBased);
}

TEST_F(FileIoTest, BufferedFileWriterTestEncrypted) {
    SharedKeyDerivationKey key = KeyDerivationKey::generate();
    auto lookup = [&key](auto k) -> SharedKeyDerivationKey {
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

TEST_F(FileIoTest, TestReadWriteGzipFile) {
    std::filesystem::path source_dir = SOURCE_PATH;
    const auto content = cb::io::loadFile(source_dir / "CMakeLists.txt");
    std::filesystem::path gzfile = file.string() + ".gz";
    auto guard = folly::makeGuard([&]() { remove(gzfile); });

    auto writer = FileWriter::create({}, gzfile, 1000, Compression::GZIP);
    EXPECT_FALSE(writer->is_encrypted());
    writer->write(content);
    writer->flush();
    writer->close();
    writer.reset();

    auto lookup = [](auto k) -> SharedKeyDerivationKey { return {}; };

    auto reader = FileReader::create(gzfile, lookup);
    EXPECT_FALSE(reader->is_encrypted());
    const auto data = reader->read();
    reader.reset();
    EXPECT_EQ(content, data);
}
