/*
 *     Copyright 2024-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace cb::crypto {
struct DataEncryptionKey;

/**
 * The FileWriter class allows for reading to a file with or without
 * encryption (depending on the file content). The primary
 * motivation for the class is to simplify the logic in the other classes
 * which wants to read both encrypted and plain versions of the files
 * depending on the configuration.
 */
class FileReader {
public:
    /**
     * Create a new instance of the FileReader
     *
     * @param path The file to read
     * @param key_lookup_function A function to look up the named key
     * @param waittime The wait time for the file to appear (if missing)
     * @return A new instance of the file reader
     * @throws std::runtime_error if an error occurs
     */
    static std::unique_ptr<FileReader> create(
            const std::filesystem::path& path,
            const std::function<std::shared_ptr<DataEncryptionKey>(
                    std::string_view)>& key_lookup_function,
            std::chrono::microseconds waittime = {});

    /// Is the file being read encrypted or not
    [[nodiscard]] virtual bool is_encrypted() const = 0;

    /**
     * Try to read the entire file in one big chunk. Note that this will
     * most likely *not* work if one tries to read an encrypted file which
     * is currently being written to due to the fact that the encrypted files
     * are chunk based, and it might hit a partial chunk.
     *
     * @return The entire file content
     * @throws std::runtime_error if an error occurs
     */
    virtual std::string read() = 0;

    /**
     * Try to read the next chunk from the file
     *
     * @return the next chunk of data (or an empty string if there was no data)
     * @throws std::runtime_error if an error occurs
     * @throws std::underflow_error if a partial chunk is detected
     */
    virtual std::string nextChunk() = 0;

    virtual ~FileReader() = default;

protected:
    FileReader() = default;
};

} // namespace cb::crypto
