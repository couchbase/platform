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
#include <fmt/format.h>
#include <folly/ScopeGuard.h>
#include <folly/portability/GTest.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>
#include <platform/file_sink.h>
#include <filesystem>
#include <random>

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

TEST_F(FileIoTest, FileWriterTestPlainClose) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file);
    EXPECT_FALSE(writer->is_encrypted());
    EXPECT_EQ(0, writer->size());
    writer->write(content);
    EXPECT_EQ(content.size(), writer->size());
    writer->flush();
    writer->close();
    EXPECT_EQ(content, cb::io::loadFile(file));
}

TEST_F(FileIoTest, FileWriterTestPlainBuffered) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file, 1024);
    EXPECT_FALSE(writer->is_encrypted());
    EXPECT_EQ(0, writer->size());
    writer->write(content);
    EXPECT_EQ(content.size(), writer->size());
    // The data should be held in the buffer until it is flushed
    EXPECT_EQ(0, std::filesystem::file_size(file));
    writer->flush();
    EXPECT_EQ(content, cb::io::loadFile(file));
    writer->close();
    EXPECT_EQ(content, cb::io::loadFile(file));
}

// Verify that buffered data isn't lost if the writer is destroyed without
// being flushed or closed
TEST_F(FileIoTest, FileWriterTestPlainBufferedNoClose) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file, 1024);
    writer->write(content);
    EXPECT_EQ(0, std::filesystem::file_size(file));
    writer.reset();
    EXPECT_EQ(content, cb::io::loadFile(file));
}

// The writer is buffered by default
TEST_F(FileIoTest, FileWriterTestPlainDefaultBuffered) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file);
    writer->write(content);
    EXPECT_EQ(0, std::filesystem::file_size(file));
    writer->flush();
    EXPECT_EQ(content, cb::io::loadFile(file));
}

// A buffer size of 0 disables buffering
TEST_F(FileIoTest, FileWriterTestPlainUnbuffered) {
    const std::string_view content = "This is the content"sv;
    auto writer = FileWriter::create({}, file, 0);
    writer->write(content);
    EXPECT_EQ(content, cb::io::loadFile(file));
}

TEST_F(FileIoTest, FileWriterOpenFailure) {
    EXPECT_THROW(FileWriter::create({},
                                    file.parent_path() / "non_existing_subdir" /
                                            "file.txt"),
                 std::system_error);
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

// Verify that buffered data isn't lost if the writer is destroyed without
// being flushed or closed
TEST_F(FileIoTest, BufferedFileWriterTestEncryptedNoClose) {
    SharedKeyDerivationKey key = KeyDerivationKey::generate();
    auto lookup = [&key](auto k) -> SharedKeyDerivationKey {
        if (key && key->id == k) {
            return key;
        }
        return {};
    };

    auto writer = FileWriter::create(key, file, 100);
    EXPECT_TRUE(writer->is_encrypted());
    for (int ii = 0; ii < 10; ++ii) {
        writer->write(std::string_view{"a", 1});
    }
    writer.reset();

    auto reader = FileReader::create(file, lookup);
    auto chunk = reader->nextChunk();
    EXPECT_EQ(std::string(10, 'a'), chunk);
}

// Verify that the gzip stream is completed if the writer is destroyed
// without being flushed or closed
TEST_F(FileIoTest, TestReadWriteGzipFileNoClose) {
    const std::string content(10000, 'a');
    std::filesystem::path gzfile = file.string() + ".gz";
    auto guard = folly::makeGuard([&]() { remove(gzfile); });

    auto writer = FileWriter::create({}, gzfile, 1000, Compression::GZIP);
    writer->write(content);
    writer.reset();

    auto lookup = [](auto k) -> SharedKeyDerivationKey { return {}; };
    auto reader = FileReader::create(gzfile, lookup);
    EXPECT_FALSE(reader->is_encrypted());
    EXPECT_EQ(content, reader->read());
}

TEST_F(FileIoTest, TestReadWriteGzipFileUnbuffered) {
    std::filesystem::path gzfile = file.string() + ".gz";
    auto guard = folly::makeGuard([&]() { remove(gzfile); });

    auto writer = FileWriter::create({}, gzfile, 0, Compression::GZIP);
    std::string content;
    for (int ii = 0; ii < 100; ++ii) {
        const auto line = fmt::format("This is line {}\n", ii);
        writer->write(line);
        content.append(line);
    }
    writer->close();
    // size() reports the number of (compressed) bytes in the file
    EXPECT_EQ(std::filesystem::file_size(gzfile), writer->size());
    EXPECT_GT(content.size(), writer->size());
    writer.reset();

    auto lookup = [](auto k) -> SharedKeyDerivationKey { return {}; };
    auto reader = FileReader::create(gzfile, lookup);
    EXPECT_EQ(content, reader->read());
}

TEST_F(FileIoTest, GzipFileWriterOpenFailure) {
    EXPECT_THROW(FileWriter::create(
                         {},
                         file.parent_path() / "non_existing_subdir" / "file.gz",
                         0,
                         Compression::GZIP),
                 std::system_error);
}

/**
 * A FileWriter which keeps the data in memory and may be told to fail
 * writes. The state lives in a separate object so that it may be inspected
 * after the writer is handed over to (and destroyed by) the writer stack.
 */
class MockFileWriter : public FileWriter {
public:
    struct State {
        std::string data;
        std::size_t num_writes = 0;
        bool fail_writes = false;
        bool closed = false;
    };

    explicit MockFileWriter(std::shared_ptr<State> state)
        : state(std::move(state)) {
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    [[nodiscard]] size_t size() const override {
        return state->data.size();
    }

    void write(std::string_view chunk) override {
        if (state->fail_writes) {
            throw std::runtime_error("MockFileWriter: injected write failure");
        }
        ++state->num_writes;
        state->data.append(chunk);
    }

    void flush() override {
    }

    void close() override {
        state->closed = true;
    }

protected:
    std::shared_ptr<State> state;
};

class ZLibStreamingWriterTest : public FileIoTest {
protected:
    /** Create an encrypted writer using ZLIB compression on the mock */
    std::unique_ptr<FileWriter> createWriter(size_t buffer_size = 0) {
        return FileWriter::wrap_with_encryption(
                key,
                std::make_unique<MockFileWriter>(state),
                buffer_size,
                Compression::ZLIB);
    }

    /** Store the data written to the mock in a file and read it back */
    std::string readBack() {
        {
            cb::io::FileSink sink(file);
            sink.sink(state->data);
            sink.close();
        }
        auto reader = FileReader::create(
                file, [this](auto) -> SharedKeyDerivationKey { return key; });
        return reader->read();
    }

    /**
     * Generate data which doesn't compress well (so that zlib produces
     * output as part of the write)
     */
    static std::string randomData(size_t size) {
        std::mt19937 generator(0xdeadbeef);
        std::string ret(size, '\0');
        for (auto& ch : ret) {
            ch = static_cast<char>(generator());
        }
        return ret;
    }

    SharedKeyDerivationKey key = KeyDerivationKey::generate();
    std::shared_ptr<MockFileWriter::State> state =
            std::make_shared<MockFileWriter::State>();
};

// Flushing an unmodified stream (repeatedly) must not upset zlib
TEST_F(ZLibStreamingWriterTest, RepeatedFlush) {
    auto writer = createWriter();
    writer->flush();
    writer->flush();
    writer->write("hello");
    writer->flush();
    writer->flush();
    writer->write(" world");
    writer->close();
    EXPECT_TRUE(state->closed);
    EXPECT_EQ("hello world", readBack());
}

// Closing a stream without writing any data produces a valid empty stream
TEST_F(ZLibStreamingWriterTest, EmptyStream) {
    auto writer = createWriter();
    writer->close();
    EXPECT_TRUE(state->closed);
    EXPECT_EQ("", readBack());
}

// The compressed data must not be passed on in tiny pieces when the chunks
// written are small (each write to the underlying writer becomes its own
// encrypted block)
TEST_F(ZLibStreamingWriterTest, SmallWritesProduceLargeBlocks) {
    std::mt19937 generator(0xdeadbeef);
    std::string content;
    auto writer = createWriter();
    for (int ii = 0; ii < 20000; ++ii) {
        const auto line = fmt::format("line {}\n", generator());
        writer->write(line);
        content.append(line);
    }
    writer->close();
    // The header + (length, block) per 64k of compressed data. Without
    // a fixed size output buffer this was several thousand writes
    EXPECT_GT(20, state->num_writes);
    EXPECT_EQ(content, readBack());
}

// A failure to write the compressed data must leave the stream in a failed
// state, and the destructor must not try to complete the broken stream
TEST_F(ZLibStreamingWriterTest, WriteFailure) {
    auto writer = createWriter();
    writer->write("hello");
    state->fail_writes = true;
    EXPECT_THROW(writer->write(randomData(1024 * 1024)), std::runtime_error);
    state->fail_writes = false;

    const auto size = state->data.size();
    EXPECT_THROW(writer->write("world"), std::runtime_error);
    writer.reset();
    EXPECT_EQ(size, state->data.size())
            << "Nothing should be written to a broken stream";
    EXPECT_FALSE(state->closed);
}

// close() must close the underlying writer even if completing the
// stream failed
TEST_F(ZLibStreamingWriterTest, CloseFailure) {
    auto writer = createWriter();
    writer->write(randomData(1024));
    state->fail_writes = true;
    EXPECT_THROW(writer->close(), std::runtime_error);
    EXPECT_TRUE(state->closed);
    state->fail_writes = false;

    const auto size = state->data.size();
    writer.reset();
    EXPECT_EQ(size, state->data.size())
            << "Nothing should be written to a broken stream";
}

// close() after a failed write must report the error and close the
// underlying writer
TEST_F(ZLibStreamingWriterTest, CloseAfterWriteFailure) {
    auto writer = createWriter();
    state->fail_writes = true;
    EXPECT_THROW(writer->write(randomData(1024 * 1024)), std::runtime_error);
    state->fail_writes = false;

    const auto size = state->data.size();
    EXPECT_THROW(writer->close(), std::runtime_error);
    EXPECT_TRUE(state->closed);
    writer.reset();
    EXPECT_EQ(size, state->data.size());
}
