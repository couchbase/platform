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
#include <folly/ScopeGuard.h>
#include <nlohmann/json.hpp>
#include <platform/dirutils.h>

#include <fstream>
#include <vector>

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
        SharedKeyDerivationKey derivation_key,
        const std::function<SharedKeyDerivationKey(std::string_view)>&
                key_lookup_function,
        const std::function<void(std::string_view, const nlohmann::json&)>&
                error,
        std::string_view unencrypted_extension,
        bool compression) {
    std::error_code ec;
    std::filesystem::directory_iterator iterator(directory, ec);
    if (ec) {
        // A missing directory means that there is nothing to rewrite
        if (ec != std::errc::no_such_file_or_directory) {
            error("Failed to iterate directory",
                  {{"path", directory.string()}, {"error", ec.message()}});
        }
        return;
    }
    // Collect the files before rewriting any of them. The rewrite creates
    // (and renames) files in the same directory, and it is unspecified
    // whether a directory_iterator observes files added after it was
    // created (the new files could otherwise be picked up and rewritten).
    std::vector<std::filesystem::path> paths;
    for (const auto& p : iterator) {
        paths.emplace_back(p.path());
    }

    for (const auto& path : paths) {
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
        // The name of the temporary file must be used as is: mktemp()
        // created the file (and its name is only unique as returned). The
        // file format is selected by the arguments to FileWriter::create()
        // and not by the file name.
        const std::filesystem::path tmpfile = cb::io::mktemp(path.string());
        std::unique_ptr<FileWriter> writer;
        auto remove_tmpfile = folly::makeGuard([&writer, &tmpfile] {
            // Close the file before removing it (required on Windows)
            writer.reset();
            std::error_code ecode;
            remove(tmpfile, ecode);
        });
        writer = FileWriter::create(
                derivation_key,
                tmpfile,
                64 * 1024,
                compression ? Compression::GZIP : Compression::None);
        std::vector<uint8_t> data(8 * 1024);

        while (!reader->eof()) {
            try {
                auto nr = reader->read(data);
                writer->write({reinterpret_cast<const char*>(data.data()), nr});
            } catch (const std::underflow_error&) {
                error("Partial chunk detected", {{"path", path.string()}});
            }
        }
        writer->flush();
        writer->close();
        reader.reset();
        // The content of tmpfile was synced as part of closing the writer,
        // but the directory must be synced for the rename to be durable.
        // When the file changes name, sync before removing the original so
        // that a crash can't leave us with neither of them.
        if (derivation_key && path.extension() != ".cef") {
            auto next = path;
            next.replace_extension(".cef");
            rename(tmpfile, next);
            cb::io::fsyncDirectory(directory);
            remove(path);
        } else if (!derivation_key && path.extension() == ".cef") {
            auto next = path;
            next.replace_extension(unencrypted_extension);
            if (compression) {
                // Note: append() would add ".gz" as a path component
                next += ".gz";
            }
            rename(tmpfile, next);
            cb::io::fsyncDirectory(directory);
            remove(path);
        } else {
            rename(tmpfile, path);
            cb::io::fsyncDirectory(directory);
        }
        remove_tmpfile.dismiss();
    }
}

} // namespace cb::crypto
