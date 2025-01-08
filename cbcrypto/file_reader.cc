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
#include <cbcrypto/file_reader.h>
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <folly/compression/Compression.h>
#include <platform/compress.h>
#include <platform/dirutils.h>
#include <platform/socket.h>
#include <zlib.h>

namespace cb::crypto {
class StringReader : public FileReader {
public:
    explicit StringReader(std::string content) : content(std::move(content)) {
    }

    [[nodiscard]] bool is_encrypted() const override {
        return false;
    }

    [[nodiscard]] std::string read() override {
        return std::move(content);
    }

    [[nodiscard]] std::string nextChunk() override {
        return std::move(content);
    }

protected:
    std::string content;
};

class EncryptedStringReader : public FileReader {
public:
    EncryptedStringReader(const DataEncryptionKey& dek,
                          EncryptedFileAssociatedData associatedData,
                          std::string_view view,
                          std::string content)
        : associatedData(associatedData),
          offset(sizeof(EncryptedFileHeader)),
          cipher(SymmetricCipher::create(dek.cipher, dek.key)),
          view(view),
          content(std::move(content)) {
    }

    [[nodiscard]] bool is_encrypted() const override {
        return true;
    }

    [[nodiscard]] std::string read() override {
        std::string ret;
        ret.reserve(view.size());
        std::string chunk;
        while (!(chunk = nextChunk()).empty()) {
            ret.append(chunk);
        }
        return ret;
    }

    [[nodiscard]] std::string nextChunk() override {
        if (view.empty()) {
            return {};
        }

        if (view.size() < sizeof(uint32_t)) {
            throw std::underflow_error(
                    "EncryptedStringReader: Missing Chunk size");
        }
        uint32_t len;
        std::memcpy(&len, view.data(), sizeof(len));
        len = ntohl(len);
        view.remove_prefix(sizeof(len));

        if (view.size() < len) {
            throw std::underflow_error(
                    "EncryptedStringReader: Missing Chunk data");
        }
        associatedData.set_offset(offset);
        auto ret = cipher->decrypt(view.substr(0, len), associatedData);
        view.remove_prefix(len);
        offset += len + sizeof(uint32_t);
        return ret;
    }

protected:
    EncryptedFileAssociatedData associatedData;
    std::size_t offset;
    std::unique_ptr<crypto::SymmetricCipher> cipher;
    std::string_view view;
    std::string content;
};

class SnappyInflateReader : public FileReader {
public:
    SnappyInflateReader(std::unique_ptr<FileReader> underlying)
        : underlying(std::move(underlying)) {
    }
    [[nodiscard]] bool is_encrypted() const override {
        return underlying->is_encrypted();
    }

    std::string read() override {
        std::string ret;
        std::string next;
        while (!(next = nextChunk()).empty()) {
            ret.append(next);
        }
        return ret;
    }

    std::string nextChunk() override {
        return inflate(underlying->nextChunk());
    }

protected:
    std::string inflate(std::string_view chunk) {
        if (chunk.empty()) {
            return {};
        }
        auto inflated = cb::compression::inflateSnappy(
                chunk, std::numeric_limits<std::size_t>::max());
        return std::string(folly::StringPiece{inflated->coalesce()});
    }

    std::unique_ptr<FileReader> underlying;
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

    std::string read() override {
        std::string ret;
        std::string next;
        while (!(next = nextChunk()).empty()) {
            ret.append(next);
        }
        return ret;
    }

    std::string nextChunk() override {
        std::string ret;
        do {
            auto chunk = underlying->nextChunk();
            if (chunk.empty()) {
                return {};
            }

            zstream.avail_in = chunk.size();
            zstream.next_in =
                    reinterpret_cast<Bytef*>(const_cast<char*>(chunk.data()));

            do {
                zstream.avail_out = buffer.size();
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

protected:
    std::unique_ptr<FileReader> underlying;
    z_stream zstream;
    std::vector<uint8_t> buffer;
};

std::unique_ptr<FileReader> FileReader::create(
        const std::filesystem::path& path,
        const std::function<SharedEncryptionKey(std::string_view)>&
                key_lookup_function,
        std::chrono::microseconds waittime) {
    // The current implementation reads the entire file into memory, but
    // later on we might want to read the file as a stream (and possibly
    // allow to wait for more data to appear if we hit a partial block)
    auto content = cb::io::loadFile(path, waittime);
    if (content.size() >= sizeof(EncryptedFileHeader)) {
        std::string headerCopy = content.substr(0, sizeof(EncryptedFileHeader));
        const auto* header =
                reinterpret_cast<EncryptedFileHeader*>(headerCopy.data());
        if (header->is_encrypted()) {
            if (!header->is_supported()) {
                throw std::runtime_error(
                        "FileReader::create(): File format not supported");
            }
            auto dek = key_lookup_function(header->get_id());
            if (!dek) {
                throw std::runtime_error(
                        fmt::format("FileReader::create({}): Missing key {}",
                                    path.string(),
                                    header->get_id()));
            }

            auto compression = header->get_compression();
            std::string_view view(content);
            view.remove_prefix(sizeof(EncryptedFileHeader));
            auto ret = std::make_unique<EncryptedStringReader>(
                    *dek, *header, view, std::move(content));

            switch (compression) {
            case Compression::None:
                return ret;

            case Compression::Snappy:
                return std::make_unique<SnappyInflateReader>(std::move(ret));
            case Compression::ZLIB:
                return std::make_unique<ZlibInflateReader>(std::move(ret));
            case Compression::GZIP:
            case Compression::ZSTD:
            case Compression::BZIP2:
                break;
            }

            throw std::runtime_error(fmt::format(
                    "InflateReader: Unsupported compression: {}", compression));
        }
    }

    return std::make_unique<StringReader>(std::move(content));
}

} // namespace cb::crypto
