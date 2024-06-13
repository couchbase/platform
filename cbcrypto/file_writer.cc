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
#include <cbcrypto/file_writer.h>
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>
#include <platform/dirutils.h>
#include <platform/socket.h>
#include <cstdio>

namespace cb::crypto {

struct FileDeleter {
    void operator()(FILE* fp) {
        fclose(fp);
    }
};

using unique_file_ptr = std::unique_ptr<FILE, FileDeleter>;

class FileWriterImpl : public FileWriter {
public:
    explicit FileWriterImpl(std::filesystem::path path)
        : filename(std::move(path)),
          file(fopen(filename.string().c_str(), "wb")) {
        if (!file) {
            throw std::system_error(errno,
                                    std::system_category(),
                                    fmt::format("cb::dek::FileWriterImpl(): "
                                                "fopen(\"{}\")",
                                                filename.string()));
        }
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    void write(const std::string_view chunk) override {
        if (fwrite(chunk.data(), chunk.size(), 1, file.get()) != 1) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("cb::dek::FileWriterImpl::write(): "
                                "fwrite(\"{}\")",
                                filename.string()));
        }
        if (ferror(file.get())) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("cb::dek::FileWriterImpl::write(): "
                                "ferror(\"{}\")",
                                filename.string()));
        }
        current_size += chunk.size();
    }

    void flush() override {
        if (fflush(file.get()) != 0) {
            throw std::system_error(
                    errno,
                    std::system_category(),
                    fmt::format("cb::dek::FileWriterImpl::fflush(): "
                                "fflush(\"{}\") failed",
                                filename.string()));
        }
    }

    [[nodiscard]] size_t size() const override {
        return current_size;
    }

protected:
    const std::filesystem::path filename;
    unique_file_ptr file;
    std::size_t current_size = 0;
};

class EncryptedFileWriterImpl : public FileWriter {
public:
    EncryptedFileWriterImpl(const std::shared_ptr<DataEncryptionKey>& dek,
                            std::unique_ptr<FileWriter> underlying,
                            size_t buffer_size)
        : buffer_size(buffer_size),
          cipher(crypto::SymmetricCipher::create(dek->cipher, dek->key)),
          underlying(std::move(underlying)) {
        if (buffer_size) {
            buffer.reserve(buffer_size);
        }

        // write the file header
        std::array<char, 7> header{0,
                                   'C',
                                   'E',
                                   'F',
                                   0,
                                   0, // Version
                                   0}; // Id size
        header.back() = gsl::narrow<char>(dek->id.size());
        this->underlying->write(std::string_view{header.data(), header.size()});
        this->underlying->write(dek->id);
    }

    [[nodiscard]] bool is_encrypted() const override {
        return true;
    }

    void write(std::string_view chunk) override {
        constexpr std::string_view::size_type max_chunk_size =
                std::numeric_limits<uint32_t>::max();
        do {
            const auto current_size = std::min(chunk.size(), max_chunk_size);
            do_write(chunk.substr(0, current_size));
            chunk.remove_prefix(current_size);
        } while (!chunk.empty());
    }

    void flush() override {
        flush_pending_data();
        underlying->flush();
    }

    [[nodiscard]] size_t size() const override {
        return underlying->size() + buffer.size();
    }

protected:
    void do_encrypt_and_write(std::string_view data) {
        const auto encrypted = cipher->encrypt(data);
        uint32_t size = htonl(gsl::narrow<uint32_t>(encrypted.size()));
        this->underlying->write(std::string_view{
                reinterpret_cast<const char*>(&size), sizeof(size)});
        this->underlying->write(encrypted);
    }

    void flush_pending_data() {
        if (!buffer.empty()) {
            do_encrypt_and_write(buffer);
            buffer.resize(0);
        }
    }

    void do_write(std::string_view view) {
        if ((buffer.size() + view.size()) < buffer_size) {
            // this fits into the buffer
            buffer.append(view);
            return;
        }

        flush_pending_data();

        if (view.size() >= buffer_size) {
            // The provided data is bigger than our buffer
            do_encrypt_and_write(view);
            return;
        }

        buffer.append(view);
    }

    const size_t buffer_size;
    std::unique_ptr<crypto::SymmetricCipher> cipher;
    std::unique_ptr<FileWriter> underlying;
    std::string buffer;
};

std::unique_ptr<FileWriter> FileWriter::create(
        const std::shared_ptr<DataEncryptionKey>& dek,
        std::filesystem::path path,
        size_t buffer_size) {
    if (dek) {
        return std::make_unique<EncryptedFileWriterImpl>(
                dek,
                std::make_unique<FileWriterImpl>(std::move(path)),
                buffer_size);
    }
    return std::make_unique<FileWriterImpl>(std::move(path));
}

} // namespace cb::crypto
