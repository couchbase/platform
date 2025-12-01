/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include "encrypted_file_associated_data.h"
#include "platform/cb_time.h"

#include <cbcrypto/common.h>
#include <cbcrypto/encrypted_file_header.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <folly/compression/Compression.h>
#include <platform/compress.h>
#include <platform/dirutils.h>
#include <platform/socket.h>
#include <zlib.h>

namespace cb::crypto {

std::string FileReader::read() {
    std::string ret;
    std::string chunk;
    while (!(chunk = nextChunk()).empty()) {
        ret.append(chunk);
    }
    return ret;
}

struct FileStreamDeleter {
    void operator()(FILE* fp) {
        fclose(fp);
    }
};
using unique_file_stream_ptr = std::unique_ptr<FILE, FileStreamDeleter>;

class FileStreamReader : public FileReader {
public:
    FileStreamReader(std::filesystem::path path, unique_file_stream_ptr stream)
        : path(std::move(path)), fp(std::move(stream)) {
        if (fseek(fp.get(), 0, SEEK_SET) == -1) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    "FileReaderImpl: fseek failed");
        }
    }

    std::size_t read(std::span<uint8_t> buffer) override {
        return read(buffer.data(), buffer.size());
    }

    std::size_t read(std::span<char> buffer) {
        return read(buffer.data(), buffer.size());
    }

    std::size_t read(void* data, size_t nbytes) {
        auto nr = fread(data, sizeof(uint8_t), nbytes, fp.get());
        const auto error = ferror(fp.get());
        if (error) {
            throw std::system_error(
                    error,
                    std::system_category(),
                    fmt::format("FileStreamReader::read(): fread failed: "
                                "nbytes:{} nr:{} feof:{}",
                                nbytes,
                                nr,
                                feof(fp.get())));
        }
        return nr;
    }

    bool eof() override {
        return feof(fp.get());
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    std::string nextChunk() override {
        if (eof()) {
            return {};
        }
        std::string buffer;
        buffer.resize(ChunkSize);
        buffer.resize(read(buffer));
        return buffer;
    }

    void set_max_allowed_chunk_size(std::size_t limit) override {
        // Ignore
    }

protected:
    std::filesystem::path path;
    unique_file_stream_ptr fp;
    static constexpr std::size_t ChunkSize = 8192;
};

class EncryptedFileReader : public FileReader {
public:
    EncryptedFileReader(const KeyDerivationKey& kdk,
                        const EncryptedFileHeader& header,
                        std::unique_ptr<FileStreamReader> underlying)
        : associated_data(header),
          offset(sizeof(EncryptedFileHeader)),
          cipher(SymmetricCipher::create(kdk.cipher, header.derive_key(kdk))),
          file(std::move(underlying)) {
        std::array<uint8_t, sizeof(EncryptedFileHeader)> buffer;
        const auto nr = file->read(buffer);
        if (nr != buffer.size()) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("EncryptedFileReader: Partial read when trying "
                                "to read encryption header: read:{} eof:{}",
                                nr,
                                file->eof()));
        }
        current_chunk = folly::IOBuf::createSeparate(1024);
    }

    [[nodiscard]] bool is_encrypted() const override {
        return true;
    }

    void set_max_allowed_chunk_size(std::size_t limit) override {
        max_allowed_chunk_size = limit;
    }

    std::size_t read(std::span<uint8_t> buffer) override {
        while (!file->eof() && (current_chunk->length() < buffer.size())) {
            do_read();
        }
        auto nbytes = std::min(current_chunk->length(), buffer.size());
        std::copy_n(current_chunk->data(), nbytes, buffer.data());
        current_chunk->trimStart(nbytes);
        return nbytes;
    }

    bool eof() override {
        return file->eof() && current_chunk->length() == 0;
    }

    std::string nextChunk() override {
        if (eof()) {
            return {};
        }
        if (current_chunk->length() == 0) {
            do_read();
        }

        std::string ret(reinterpret_cast<const char*>(current_chunk->data()),
                        current_chunk->length());
        current_chunk->trimStart(current_chunk->length());
        return ret;
    }

protected:
    /// Get the next chunk of data and decrypt it. See the Chunk section
    /// in EncryptedFileFormat.md for a description of the chunk layout
    void do_read() {
        uint32_t chunk_size;
        auto nr = file->read(&chunk_size, sizeof(uint32_t));
        if (nr == 0) {
            return;
        }
        if (nr != sizeof(uint32_t)) {
            throw std::underflow_error(
                    "EncryptedFileReader: Missing Chunk size");
        }
        chunk_size = ntohl(chunk_size);
        if (static_cast<std::size_t>(chunk_size) > max_allowed_chunk_size) {
            throw std::runtime_error(
                    fmt::format("EncryptedFileReader: Chunk size ({}) exceeds "
                                "the maximum allowed chunk size ({})",
                                chunk_size,
                                max_allowed_chunk_size));
        }

        std::string buffer;
        buffer.resize(chunk_size);
        if (file->read(buffer) != buffer.size()) {
            throw std::underflow_error(
                    "EncryptedFileReader: Missing Chunk data");
        }

        associated_data.set_offset(offset);
        auto decrypted = cipher->decrypt(buffer, associated_data);
        offset += chunk_size + sizeof(uint32_t);
        if (current_chunk->tailroom() < decrypted.size()) {
            current_chunk->reserve(0, decrypted.size());
            Expects(current_chunk->tailroom() >= decrypted.size());
        }
        std::copy_n(decrypted.data(),
                    decrypted.size(),
                    current_chunk->writableTail());
        current_chunk->append(decrypted.size());
    }

    EncryptedFileAssociatedData associated_data;
    std::size_t offset;
    std::unique_ptr<SymmetricCipher> cipher;
    std::unique_ptr<FileStreamReader> file;
    std::unique_ptr<folly::IOBuf> current_chunk;
    std::size_t max_allowed_chunk_size = default_max_allowed_chunk_size;
};

class SnappyInflateReader : public FileReader {
public:
    SnappyInflateReader(std::unique_ptr<FileReader> underlying)
        : underlying(std::move(underlying)) {
    }
    [[nodiscard]] bool is_encrypted() const override {
        return underlying->is_encrypted();
    }

    void set_max_allowed_chunk_size(std::size_t limit) override {
        underlying->set_max_allowed_chunk_size(limit);
    }

    std::string nextChunk() override {
        if (eof()) {
            return {};
        }

        if (current_chunk.empty()) {
            return inflate(underlying->nextChunk());
        }
        std::string ret;
        ret.swap(current_chunk);
        return ret;
    }

    std::size_t read(std::span<uint8_t> buffer) override {
        std::size_t nr = 0;
        do {
            auto chunk = do_read(buffer);
            nr += chunk;
            buffer = buffer.subspan(chunk);
        } while (!buffer.empty() && !eof());
        return nr;
    }

    bool eof() override {
        return current_chunk.empty() && underlying->eof();
    }

protected:
    std::size_t do_read(std::span<uint8_t> buffer) {
        auto data = nextChunk();
        auto nbytes = std::min(buffer.size(), data.size());
        std::copy_n(data.data(), nbytes, buffer.data());
        if (nbytes < data.size()) {
            current_chunk = data.substr(nbytes);
        }
        return nbytes;
    }

    std::string inflate(std::string_view chunk) {
        if (chunk.empty()) {
            return {};
        }
        auto inflated = cb::compression::inflateSnappy(
                chunk, std::numeric_limits<std::size_t>::max());
        return std::string(folly::StringPiece{inflated->coalesce()});
    }

    std::unique_ptr<FileReader> underlying;
    std::string current_chunk;
};

class ZlibInflateReader : public FileReader {
public:
    ZlibInflateReader(std::unique_ptr<FileReader> underlying)
        : underlying(std::move(underlying)) {
        std::memset(&zstream, 0, sizeof(zstream));
        if (inflateInit(&zstream) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib");
        }
        buffer.resize(1024 * 1024);
    }

    [[nodiscard]] bool is_encrypted() const override {
        return underlying->is_encrypted();
    }

    void set_max_allowed_chunk_size(std::size_t limit) override {
        underlying->set_max_allowed_chunk_size(limit);
    }

    std::string nextChunk() override {
        if (eof()) {
            return {};
        }
        std::string ret;

        if (!current_chunk.empty()) {
            ret.swap(current_chunk);
            return ret;
        }

        do {
            auto chunk = underlying->nextChunk();
            if (chunk.empty()) {
                return {};
            }

            zstream.avail_in = gsl::narrow_cast<uInt>(chunk.size());
            zstream.next_in =
                    reinterpret_cast<Bytef*>(const_cast<char*>(chunk.data()));

            do {
                zstream.avail_out = gsl::narrow_cast<uInt>(buffer.size());
                zstream.next_out = buffer.data();
                auto rc = inflate(&zstream, Z_NO_FLUSH);
                Expects(rc == Z_OK || rc == Z_STREAM_END);
                auto nbytes = buffer.size() - zstream.avail_out;
                ret.append(reinterpret_cast<const char*>(buffer.data()),
                           nbytes);
            } while (zstream.avail_out == 0);
        } while (ret.empty());
        return ret;
    }

    ~ZlibInflateReader() override {
        inflateEnd(&zstream);
    }

    std::size_t read(std::span<uint8_t> buffer) override {
        std::size_t nr = 0;
        do {
            auto chunk = do_read(buffer);
            nr += chunk;
            buffer = buffer.subspan(chunk);
        } while (!buffer.empty() && !eof());
        return nr;
    }

    bool eof() override {
        return current_chunk.empty() && underlying->eof();
    }

protected:
    std::size_t do_read(std::span<uint8_t> buffer) {
        auto data = nextChunk();
        auto nbytes = std::min(buffer.size(), data.size());
        std::copy_n(data.data(), nbytes, buffer.data());
        if (nbytes < data.size()) {
            current_chunk = data.substr(nbytes);
        }
        return nbytes;
    }

    std::unique_ptr<FileReader> underlying;
    z_stream zstream;
    std::vector<uint8_t> buffer;
    std::string current_chunk;
};

std::unique_ptr<FileReader> FileReader::create(
        const std::filesystem::path& path,
        const std::function<SharedKeyDerivationKey(std::string_view)>&
                key_lookup_function,
        std::chrono::microseconds waittime) {
    const auto timeout =
            cb::time::steady_clock::now() + std::chrono::microseconds(waittime);
    FILE* fp;
    while ((fp = fopen(path.string().c_str(), "rb")) == nullptr) {
        if (errno != ENOENT) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("Failed to open {}", path.string()));
        }
        if (waittime == std::chrono::microseconds::zero() ||
            cb::time::steady_clock::now() > timeout) {
            throw std::system_error(
                    ENOENT, std::system_category(), "Failed to check for file");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    unique_file_stream_ptr file_stream(fp);

    bool encrypted = false;
    SharedKeyDerivationKey kdk;
    Compression compression = Compression::None;

    std::vector<uint8_t> buffer(sizeof(EncryptedFileHeader));
    const EncryptedFileHeader* header =
            reinterpret_cast<EncryptedFileHeader*>(buffer.data());
    auto nr = fread(buffer.data(), sizeof(EncryptedFileHeader), 1, fp);

    if (nr == 1) {
        if (header->is_encrypted()) {
            if (!header->is_supported()) {
                throw std::runtime_error(fmt::format(
                        "FileReader::create({}): File format not supported",
                        path.string()));
            }
            kdk = key_lookup_function(header->get_id());
            if (!kdk) {
                throw std::runtime_error(
                        fmt::format("FileReader::create({}): Missing key {}",
                                    path.string(),
                                    header->get_id()));
            }
            compression = header->get_compression();
            encrypted = true;
        }
    }

    if (encrypted) {
        auto enc_file_reader = std::make_unique<EncryptedFileReader>(
                *kdk,
                *header,
                std::make_unique<FileStreamReader>(path,
                                                   std::move(file_stream)));

        switch (compression) {
        case Compression::None:
            return enc_file_reader;

        case Compression::Snappy:
            return std::make_unique<SnappyInflateReader>(
                    std::move(enc_file_reader));
        case Compression::ZLIB:
            return std::make_unique<ZlibInflateReader>(
                    std::move(enc_file_reader));
        case Compression::GZIP:
        case Compression::ZSTD:
        case Compression::BZIP2:
            break;
        }

        throw std::runtime_error(fmt::format(
                "InflateReader: Unsupported compression: {}", compression));
    }

    return std::make_unique<FileStreamReader>(path, std::move(file_stream));
}

} // namespace cb::crypto
