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
#include <platform/file_sink.h>
#include <platform/socket.h>
#include <zlib.h>

namespace cb::crypto {

/**
 * The FileWriterImpl is the actual implementation of the FileWriter
 * interface. It is used to write data to a file on disk, and fsync the
 * data to disk as part of closing the file.
 */
class FileWriterImpl : public FileWriter {
public:
    explicit FileWriterImpl(std::filesystem::path path)
        : file(std::move(path)) {
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    [[nodiscard]] size_t size() const override {
        return file.getBytesWritten();
    }

    void write(std::string_view chunk) override {
        file.sink(chunk);
    }

    void flush() override {
        // FileSink is unbuffered so all data has already been handed over
        // to the OS. The data is synced to disk as part of close() (we
        // don't want to fsync on every flush)
    }

    void close() override {
        file.close();
    }

protected:
    cb::io::FileSink file;
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

    ~BufferedWriter() override {
        // User didn't explicitly flush or close the stream so we need to
        // pass on any pending data to avoid losing it. The underlying
        // writer is destroyed after this destructor runs (it is a member
        // of the base class) so it is still valid here. Catch exceptions
        // as a destructor should not throw.
        try {
            flush_pending_data();
        } catch (const std::exception&) {
        }
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
    /** The format of the compressed stream */
    enum class Format {
        /** zlib format (RFC 1950) */
        Zlib,
        /** gzip format (RFC 1952), readable by gzip(1) and gzread() */
        Gzip
    };

    ZLibStreamingWriter(std::unique_ptr<FileWriter> underlying,
                        Format format = Format::Zlib)
        : StackedWriter(std::move(underlying)) {
        std::memset(&zstream, 0, sizeof(zstream));
        // Adding 16 to the window bits makes zlib write a gzip header and
        // trailer instead of the zlib wrapper. 8 is the default memLevel
        // used by deflateInit()
        const int window_bits =
                format == Format::Gzip ? MAX_WBITS + 16 : MAX_WBITS;
        const auto rc = deflateInit2(&zstream,
                                     Z_DEFAULT_COMPRESSION,
                                     Z_DEFLATED,
                                     window_bits,
                                     8,
                                     Z_DEFAULT_STRATEGY);
        if (rc != Z_OK) {
            throw std::runtime_error(
                    fmt::format("ZLibStreamingWriter::ZLibStreamingWriter(): "
                                "Failed to initialize zlib: {}",
                                rc));
        }
    }

    void flush() override {
        // We don't want to flush the zlib stream here as it would
        // reduce the compression ratio.
    }

    void close() override {
        try {
            do_close();
        } catch (const std::exception&) {
            // Close the underlying writer to release the file, but report
            // the original error
            try {
                underlying->close();
            } catch (const std::exception&) {
            }
            throw;
        }
        underlying->close();
    }

    ~ZLibStreamingWriter() override {
        // Don't try to complete a stream which is known to be broken (part
        // of the compressed data was lost). Leaving it truncated lets the
        // reader detect the error instead of getting a complete looking
        // stream with a hole in it
        if (!closed && !failed) {
            try {
                do_close();
            } catch (const std::exception&) {
            }
        }
        deflateEnd(&zstream);
    }

protected:
    /**
     * The size of the buffer used to receive the output from deflate().
     * It is independent of the size of the chunks written to avoid passing
     * on the compressed data in tiny pieces when the chunks are small (and
     * allocating huge buffers when they are big)
     */
    constexpr static size_t OutputBufferSize = 64 * 1024;

    /** Throw an exception if a previous error left the stream broken */
    void check_not_failed(std::string_view method) const {
        if (failed) {
            throw std::runtime_error(fmt::format(
                    "ZLibStreamingWriter::{}(): The stream is in a failed "
                    "state due to a previous error",
                    method));
        }
    }

    /** Prepare the output buffer to receive output from deflate() */
    void reset_output() {
        zstream.avail_out = gsl::narrow_cast<uInt>(output.size());
        zstream.next_out = output.data();
    }

    /** Pass the output produced by deflate() on to the underlying writer */
    void write_output() {
        const auto nbytes = output.size() - zstream.avail_out;
        if (nbytes) {
            underlying->write(std::string_view{
                    reinterpret_cast<const char*>(output.data()), nbytes});
        }
    }

    void do_close() {
        Expects(!closed);
        closed = true;
        check_not_failed("do_close");
        zstream.avail_in = 0;
        zstream.next_in = nullptr;
        int rc = Z_OK;
        try {
            do {
                reset_output();
                rc = deflate(&zstream, Z_FINISH);
                write_output();
            } while (rc == Z_OK);
        } catch (const std::exception&) {
            // Part of the compressed data never reached the underlying writer
            failed = true;
            throw;
        }
        if (rc == Z_STREAM_END) {
            return;
        }
        failed = true;
        throw std::runtime_error(
                fmt::format("ZLibStreamingWriter::do_close(): Failed to "
                            "deflate data with Z_FINISH: {}",
                            rc));
    }

    void do_write(std::string_view data) override {
        Expects(!closed);
        check_not_failed("do_write");
        zstream.avail_in = gsl::narrow_cast<uInt>(data.size());
        zstream.next_in =
                reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

        try {
            do {
                reset_output();
                // Z_BUF_ERROR is not an error; it is returned when the
                // output buffer was filled exactly by the previous call and
                // there was nothing more to do
                const auto status = deflate(&zstream, Z_NO_FLUSH);
                if (status != Z_OK && status != Z_BUF_ERROR) {
                    throw std::runtime_error(fmt::format(
                            "ZLibStreamingWriter::do_write(): Failed to "
                            "deflate data (Z_NO_FLUSH): {}",
                            status));
                }
                write_output();
            } while (zstream.avail_out == 0);
        } catch (const std::exception&) {
            // zlib may have consumed input (and produced output) which
            // never reached the underlying writer
            failed = true;
            throw;
        }
        Expects(zstream.avail_in == 0);
    }

    bool closed = false;
    /** Set if an error caused data to be lost and the stream to be broken */
    bool failed = false;
    z_stream zstream;
    std::vector<uint8_t> output = std::vector<uint8_t>(OutputBufferSize);
};

class EncryptedWriter : public StackedWriter {
public:
    EncryptedWriter(const SharedKeyDerivationKey& kdk,
                    const EncryptedFileHeader& header,
                    std::unique_ptr<FileWriter> underlying)
        : StackedWriter(std::move(underlying)),
          associatedData(std::make_unique<EncryptedFileAssociatedData>(header)),
          cipher(SymmetricCipher::create(kdk->cipher,
                                         header.derive_key(*kdk))) {
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
    std::unique_ptr<FileWriter> ret =
            std::make_unique<FileWriterImpl>(std::move(path));
    if (!kdk) {
        if (compression == Compression::GZIP) {
            // Plain files use the gzip file format
            ret = std::make_unique<ZLibStreamingWriter>(
                    std::move(ret), ZLibStreamingWriter::Format::Gzip);
        }
        if (buffer_size != 0) {
            ret = std::make_unique<BufferedWriter>(std::move(ret), buffer_size);
        }
        return ret;
    }
    return wrap_with_encryption(kdk, std::move(ret), buffer_size, compression);
}

std::unique_ptr<FileWriter> FileWriter::wrap_with_encryption(
        const SharedKeyDerivationKey& kdk,
        std::unique_ptr<FileWriter> ret,
        size_t buffer_size,
        Compression compression) {
    Expects(kdk);

    // GZIP is not supported in encrypted files, map to ZLIB
    if (compression == Compression::GZIP) {
        compression = Compression::ZLIB;
    }

    EncryptedFileHeader header(kdk->id, kdk->derivationMethod, compression);
    if (kdk->derivationMethod == KeyDerivationMethod::PasswordBased) {
        // set a default number of iterations
        header.set_pbkdf_iterations(128 * 1024);
    }
    ret->write(header);

    // time to build up the stack of writers

    ret = std::make_unique<EncryptedWriter>(kdk, header, std::move(ret));

    switch (compression) {
    case Compression::None:
        break;
    case Compression::GZIP:
        folly::assume_unreachable();
    case Compression::ZLIB:
        ret = std::make_unique<ZLibStreamingWriter>(std::move(ret));
        break;
    case Compression::Snappy:
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
