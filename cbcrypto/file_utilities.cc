/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#include <cbcrypto/encrypted_file_header.h>
#include <cbcrypto/file_reader.h>
#include <cbcrypto/file_utilities.h>
#include <cbcrypto/file_writer.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>

#include <fstream>

namespace cb::crypto {

static std::string getEncryptionKey(const std::filesystem::path& path) {
    std::array<char, sizeof(EncryptedFileHeader)> buffer;
    auto size = file_size(path);
    if (size < buffer.size()) {
        // No header present
        return {};
    }

    std::ifstream input;
    input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    input.open(path, std::ios::binary);
    input.read(buffer.data(), buffer.size());
    input.close();

    auto* header = reinterpret_cast<EncryptedFileHeader*>(buffer.data());
    if (!header->is_encrypted()) {
        throw std::logic_error(
                "File with .cef extension does not have correct magic");
    }

    if (!header->is_supported()) {
        throw std::logic_error("File with .cef extension is not supported");
    }
    return std::string{header->get_id()};
}

std::unordered_set<std::string> findDeksInUse(
        const std::filesystem::path& directory,
        const std::function<bool(const std::filesystem::path&)>& filefilter,
        const std::function<void(std::string_view, const nlohmann::json&)>&
                error) {
    std::unordered_set<std::string> deks;
    std::error_code ec;
    for (const auto& p : std::filesystem::directory_iterator(directory, ec)) {
        auto path = p.path();
        if (!filefilter(path)) {
            continue;
        }

        try {
            auto key = getEncryptionKey(path);
            if (!key.empty()) {
                deks.insert(key);
            }
        } catch (const std::exception& e) {
            error("Failed to get deks from",
                  {{"path", path.string()}, {"error", e.what()}});
        }
    }

    if (ec) {
        error("Error occurred while traversing directory",
              {{"path", directory.string()}, {"error", ec.message()}});
    }

    return deks;
}

void maybeRewriteFiles(
        const std::filesystem::path& directory,
        const std::function<bool(const std::filesystem::path&,
                                 std::string_view)>& filefilter,
        SharedEncryptionKey encryption_key,
        const std::function<SharedEncryptionKey(std::string_view)>&
                key_lookup_function,
        const std::function<void(std::string_view, const nlohmann::json&)>&
                error,
        std::string_view unencrypted_extension) {
    std::error_code ec;
    for (const auto& p : std::filesystem::directory_iterator(directory, ec)) {
        auto path = p.path();
        std::string key;
        if (path.extension() == ".cef") {
            try {
                key = getEncryptionKey(path);
            } catch (const std::exception& e) {
                error("Failed to get deks from",
                      {{"path", path.string()}, {"error", e.what()}});
                continue;
            }
        }

        if (!filefilter(path, key)) {
            continue;
        }

        auto reader = FileReader::create(path, key_lookup_function);
        std::filesystem::path tmpfile = cb::io::mktemp(path.string());
        auto writer = FileWriter::create(encryption_key, tmpfile, 64 * 1024);

        bool eof = false;
        do {
            try {
                auto message = reader->nextChunk();
                if (message.empty()) {
                    eof = true;
                } else {
                    writer->write(message);
                }
            } catch (const std::underflow_error&) {
                error("Partial chunk detected", {{"path", path.string()}});
                eof = true;
            }
        } while (!eof);
        writer->flush();
        writer->close();
        if (encryption_key && path.extension() != ".cef") {
            auto next = path;
            next.replace_extension(".cef");
            rename(tmpfile, next);
            remove(path);
        } else if (!encryption_key && path.extension() == ".cef") {
            auto next = path;
            next.replace_extension(unencrypted_extension);
            rename(tmpfile, next);
            remove(path);
        } else {
            rename(tmpfile, path);
        }
    }
}

} // namespace cb::crypto
