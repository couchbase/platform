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
#include <cbcrypto/file_utilities.h>
#include <nlohmann/json.hpp>
#include <fstream>

namespace cb::crypto {

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

        std::array<char, sizeof(cb::crypto::EncryptedFileHeader)> buffer;
        try {
            auto size = file_size(path);
            if (size < buffer.size()) {
                // No header present
                continue;
            }

            std::ifstream input;
            input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            input.open(path, std::ios::binary);
            input.read(buffer.data(), buffer.size());
            input.close();
        } catch (const std::exception& e) {
            error("Failed to get deks from",
                  {{"path", path.string()}, {"error", e.what()}});
            continue;
        }

        auto* header = reinterpret_cast<cb::crypto::EncryptedFileHeader*>(
                buffer.data());
        if (!header->is_encrypted()) {
            error("Logfile with .cef extension does not have correct magic",
                  {{"path", path.string()}});
            continue;
        }

        if (!header->is_supported()) {
            error("Logfile with .cef extension is not supported",
                  {{"path", path.string()}});
            continue;
        }
        deks.insert(std::string(header->get_id()));
    }

    if (ec) {
        error("Error occurred while traversing directory",
              {{"path", directory.string()}, {"error", ec.message()}});
    }

    return deks;
}

} // namespace cb::crypto
