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
#include <cbcrypto/symmetric.h>
#include <fmt/format.h>
#include <gsl/gsl-lite.hpp>
#include <platform/dirutils.h>
#include <platform/socket.h>

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
                          std::string_view view,
                          std::string content)
        : cipher(crypto::SymmetricCipher::create(dek.cipher, dek.key)),
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

        auto ret = cipher->decrypt(view.substr(0, len));
        view.remove_prefix(len);
        return ret;
    }

protected:
    std::unique_ptr<crypto::SymmetricCipher> cipher;
    std::string_view view;
    std::string content;
};

std::unique_ptr<FileReader> FileReader::create(
        const std::filesystem::path& path,
        const std::function<std::shared_ptr<DataEncryptionKey>(
                std::string_view)>& key_lookup_function,
        std::chrono::microseconds waittime) {
    // The current implementation reads the entire file into memory, but
    // later on we might want to read the file as a stream (and possibly
    // allow to wait for more data to appear if we hit a partial block)
    auto content = cb::io::loadFile(path.string(), waittime);
    if (content.rfind(std::string_view{"\0CEF\0", 5}, 0) == 0) {
        // This is an encrypted file
        std::string_view view(content);
        if (view.size() < 7) {
            throw std::runtime_error(
                    fmt::format("FileReader::create(\"{}\"): partial header",
                                path.string()));
        }
        view.remove_prefix(5);
        if (view.front() != '\0') {
            throw std::runtime_error("Unknown version");
        }
        view.remove_prefix(1);
        const auto idlen = static_cast<uint8_t>(view.front());
        view.remove_prefix(1);
        if (view.size() < idlen) {
            throw std::runtime_error(fmt::format(
                    "FileReader::create(\"{}\"): partial header. Missing key",
                    path.string()));
        }
        auto id = view.substr(0, idlen);
        view.remove_prefix(idlen);

        auto dek = key_lookup_function(id);
        if (!dek) {
            throw std::runtime_error(
                    fmt::format("FileReader::create({}): Missing key {}",
                                path.string(),
                                id));
        }

        return std::make_unique<EncryptedStringReader>(
                *dek, view, std::move(content));
    }
    return std::make_unique<StringReader>(std::move(content));
}

} // namespace cb::crypto
