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

#include <cbcrypto/common.h>
#include <cbcrypto/encrypted_file_header.h>
#include <cbcrypto/file_writer.h>
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <folly/compression/Compression.h>
#include <folly/io/IOBuf.h>
#include <gsl/gsl-lite.hpp>
#include <platform/compress.h>
#include <platform/dirutils.h>
#include <platform/socket.h>
#include <zlib.h>
#include <fstream>

namespace cb::crypto {

/**
 * The FileWriterImpl is the actual implementation of the FileWriter
 * interface. It is used to write data to a file on disk.
 */
class FileWriterImpl : public FileWriter {
public:
    FileWriterImpl(std::ofstream fstream) : file(std::move(fstream)) {
        if (!file.is_open()) {
            throw std::invalid_argument("file should be open");
        }
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    [[nodiscard]] size_t size() const override {
        return current_size;
    }

    void write(std::string_view chunk) override {
        Expects(file.is_open());
        file.write(chunk.data(), chunk.size());
        current_size += chunk.size();
    }

    void flush() override {
        Expects(file.is_open());
        file.flush();
    }

    void close() override {
        Expects(file.is_open());
        file.close();
    }

    ~FileWriterImpl() override = default;

protected:
    const std::filesystem::path filename;
    std::ofstream file;
    std::size_t current_size = 0;
};

class StackedWriter : public FileWriter {
public:
    StackedWriter(std::unique_ptr<FileWriter> underlying)
        : underlying(std::move(underlying)) {
    }

    [[nodiscard]] bool is_encrypted() const override {
        return underlying->is_encrypted();
    }

    void write(std::string_view chunk) override {
        if (chunk.empty()) {
            // ignore empty chunks
            return;
        }

        constexpr std::string_view::size_type max_chunk_size =
                std::numeric_limits<uint32_t>::max();
        do {
            const auto current_size = std::min(chunk.size(), max_chunk_size);
            do_write(chunk.substr(0, current_size));
            chunk.remove_prefix(current_size);
        } while (!chunk.empty());
    }

    void flush() override {
        underlying->flush();
    }

    void close() override {
        flush();
        underlying->close();
    }

    [[nodiscard]] size_t size() const override {
        return underlying->size();
    }

protected:
    virtual void do_write(std::string_view view) = 0;
    std::unique_ptr<FileWriter> underlying;
};

class BufferedWriter : public StackedWriter {
public:
    BufferedWriter(std::unique_ptr<FileWriter> underlying, size_t buffer_size)
        : StackedWriter(std::move(underlying)), buffer_size(buffer_size) {
        buffer.reserve(buffer_size);
    }

    void flush() override {
        flush_pending_data();
        underlying->flush();
    }

    [[nodiscard]] size_t size() const override {
        return underlying->size() + buffer.size();
    }

protected:
    void flush_pending_data() {
        if (!buffer.empty()) {
            underlying->write(buffer);
            buffer.resize(0);
        }
    }

    void do_write(std::string_view view) override {
        if ((buffer.size() + view.size()) < buffer_size) {
            // this fits into the buffer
            buffer.append(view);
            return;
        }

        flush_pending_data();

        if (view.size() >= buffer_size) {
            // The provided data is bigger than our buffer
            underlying->write(view);
            return;
        }

        buffer.append(view);
    }

    const size_t buffer_size;
    std::string buffer;
};

class CompressionWriter : public StackedWriter {
public:
    CompressionWriter(std::unique_ptr<FileWriter> underlying,
                      Compression compression)
        : StackedWriter(std::move(underlying)), compression(compression) {
    }

protected:
    void do_write(std::string_view data) override {
        std::unique_ptr<folly::IOBuf> iobuf;
        iobuf = deflate(data);
        data = folly::StringPiece{iobuf->coalesce()};
        this->underlying->write(data);
    }

    std::unique_ptr<folly::IOBuf> deflate(std::string_view chunk) {
        if (chunk.empty()) {
            return {};
        }

        folly::io::CodecType codec;

        switch (compression) {
        case Compression::Snappy:
            codec = folly::io::CodecType::SNAPPY;
            break;
        case Compression::ZLIB:
            codec = folly::io::CodecType::ZLIB;
            break;
        case Compression::GZIP:
            codec = folly::io::CodecType::GZIP;
            break;
        case Compression::ZSTD:
            codec = folly::io::CodecType::ZSTD;
            break;
        case Compression::BZIP2:
            codec = folly::io::CodecType::BZIP2;
            break;
        default:
            throw std::runtime_error(fmt::format(
                    "CompressionWriter: Unsupported compression: {}",
                    compression));
        }

        return cb::compression::deflate(codec, chunk);
    }

    const Compression compression;
};

class ZLibStreamingWriter : public StackedWriter {
public:
    ZLibStreamingWriter(std::unique_ptr<FileWriter> underlying)
        : StackedWriter(std::move(underlying)) {
        std::memset(&zstream, 0, sizeof(zstream));
        if (deflateInit(&zstream, Z_DEFAULT_COMPRESSION) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib");
        }
    }

    void flush() override {
        Expects(!closed);
        zstream.avail_in = 0;
        zstream.next_in = nullptr;

        std::vector<uint8_t> buffer(1024);
        zstream.avail_out = gsl::narrow_cast<uInt>(buffer.size());
        zstream.next_out = buffer.data();

        // (given that the output size is the same as the input size I expect
        // that we should be able to compress the data in one go)
        if (deflate(&zstream, Z_FULL_FLUSH) != Z_OK) {
            throw std::runtime_error("Failed to deflate data");
        }
        auto nbytes = buffer.size() - zstream.avail_out;
        this->underlying->write(std::string_view{
                reinterpret_cast<const char*>(buffer.data()), nbytes});
        this->underlying->flush();
    }

    void close() override {
        do_close();
    }

    ~ZLibStreamingWriter() override {
        if (!closed) {
            try {
                do_close();
            } catch (const std::exception&) {
            }
        }
        deflateEnd(&zstream);
    }

protected:
    void do_close() {
        Expects(!closed);
        zstream.avail_in = 0;
        zstream.next_in = nullptr;
        std::vector<uint8_t> buffer(1024);
        int rc = Z_OK;
        do {
            zstream.avail_out = gsl::narrow_cast<uInt>(buffer.size());
            zstream.next_out = buffer.data();
            rc = deflate(&zstream, Z_FINISH);
            auto nbytes = buffer.size() - zstream.avail_out;
            if (nbytes) {
                this->underlying->write(std::string_view{
                        reinterpret_cast<const char*>(buffer.data()), nbytes});
                this->underlying->flush();
            }
        } while (rc == Z_OK);
        closed = true;
        if (rc == Z_STREAM_END) {
            return;
        }
        throw std::runtime_error(
                fmt::format("Failed to deflate data with Z_FINISH: {}", rc));
    }

    void do_write(std::string_view data) override {
        Expects(!closed);
        zstream.avail_in = gsl::narrow_cast<uInt>(data.size());
        zstream.next_in =
                reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

        std::vector<uint8_t> buffer(data.size());
        do {
            zstream.avail_out = gsl::narrow_cast<uInt>(buffer.size());
            zstream.next_out = buffer.data();

            auto status = deflate(&zstream, Z_NO_FLUSH);
            if (status == Z_STREAM_ERROR) {
                throw std::runtime_error("Failed to deflate data (Z_NO_FLUSH)");
            }
            const auto nbytes = buffer.size() - zstream.avail_out;
            this->underlying->write(std::string_view{
                    reinterpret_cast<const char*>(buffer.data()), nbytes});
        } while (zstream.avail_out == 0);
        Expects(zstream.avail_in == 0);
    }

    bool closed = false;
    z_stream zstream;
};

class EncryptedWriter : public StackedWriter {
public:
    EncryptedWriter(const SharedKeyDerivationKey& kdk,
                    const EncryptedFileHeader& header,
                    std::unique_ptr<FileWriter> underlying)
        : StackedWriter(std::move(underlying)),
          associatedData(std::make_unique<EncryptedFileAssociatedData>(header)),
          cipher(SymmetricCipher::create(kdk->cipher, kdk->derivationKey)) {
    }

    bool is_encrypted() const override {
        return true;
    }

protected:
    void do_write(std::string_view data) override {
        associatedData->set_offset(underlying->size());
        const auto encrypted = cipher->encrypt(data, *associatedData);
        uint32_t size = htonl(gsl::narrow<uint32_t>(encrypted.size()));
        this->underlying->write(std::string_view{
                reinterpret_cast<const char*>(&size), sizeof(size)});
        this->underlying->write(encrypted);
    }

    std::unique_ptr<EncryptedFileAssociatedData> associatedData;
    std::unique_ptr<SymmetricCipher> cipher;
};

std::unique_ptr<FileWriter> FileWriter::create(
        const SharedKeyDerivationKey& kdk,
        std::filesystem::path path,
        size_t buffer_size,
        Compression compression) {
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(path.string(), std::ios_base::trunc | std::ios_base::binary);

    std::unique_ptr<FileWriter> ret =
            std::make_unique<FileWriterImpl>(std::move(file));
    if (!kdk) {
        return ret;
    }

    EncryptedFileHeader header(kdk->id, cb::uuid::random(), compression);
    ret->write(header);

    // time to build up the stack of writers

    ret = std::make_unique<EncryptedWriter>(kdk, header, std::move(ret));

    switch (compression) {
    case Compression::None:
        break;
    case Compression::ZLIB:
        ret = std::make_unique<ZLibStreamingWriter>(std::move(ret));
        break;
    case Compression::Snappy:
    case Compression::GZIP:
    case Compression::ZSTD:
    case Compression::BZIP2:
        ret = std::make_unique<CompressionWriter>(std::move(ret), compression);
        break;
    }

    if (buffer_size != 0) {
        ret = std::make_unique<BufferedWriter>(std::move(ret), buffer_size);
    }
    return ret;
}

} // namespace cb::crypto
